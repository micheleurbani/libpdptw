#include "algorithms/alns.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <utility>
#include <vector>

extern "C" {
#include "config.h"
}

// ===================================================================== //
//  Internal helpers (file-local)                                        //
// ===================================================================== //
namespace {

// Penalty weights of the local-search cost function (eq. 1 of Li & Lim),
// reused here as ALNS's own objective for scoring trial solutions.
struct CostWeights {
    double nv;
    double dist;
    double sd;
    double wt;
};

double cost(Solution* S, const CostWeights& w) {
    return get_solution_cost(S, w.nv, w.dist, w.sd, w.wt);
}

// ALNS's SA-acceptance objective (design decision 1): the raw solution cost
// plus a flat penalty per still-unassigned request. A trial solution may
// carry unassigned requests transiently through the SA walk, but it can
// never become the returned "best" (gated on n_ujobs == 0 at the call site).
double eval_cost(Solution* S, const CostWeights& w, double w_unserved) {
    return cost(S, w) + w_unserved * S->n_ujobs;
}

// Roulette-wheel selection proportional to a 3-entry weight vector.
int select_index(const std::array<double, 3>& weights, std::mt19937& rng) {
    double total = weights[0] + weights[1] + weights[2];
    std::uniform_real_distribution<double> unif(0.0, total);
    double draw = unif(rng);
    double acc = 0.0;
    for (int i = 0; i < 2; i++) {
        acc += weights[i];
        if (draw <= acc) return i;
    }
    return 2;
}

enum DestroyOp { D_RANDOM = 0, D_WORST = 1, D_SHAW = 2, N_DESTROY = 3 };
enum RepairOp { R_GREEDY = 0, R_REGRET2 = 1, R_REGRET3 = 2, N_REPAIR = 3 };

// --- Shared request-bank bookkeeping ------------------------------------ //

int total_requests(Solution* S) { return S->problem->N / 2; }

// How many requests a destroy call should remove this iteration: a fraction
// of all requests in the instance, capped by how many are actually routed
// right now (a prior repair may have left some already in S->ujobs).
int sample_removal_count(Solution* S, double frac_min, double frac_max,
                          std::mt19937& rng) {
    int n = total_requests(S);
    int routed = n - S->n_ujobs;
    if (routed <= 0) return 0;
    int lo = std::max(1, (int)std::floor(frac_min * n));
    int hi = std::max(lo, (int)std::ceil(frac_max * n));
    hi = std::min(hi, routed);
    lo = std::min(lo, hi);
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

// Move `job` out of route `r` and onto the unassigned-request bank.
void remove_request(Solution* S, Route* r, int* job) {
    remove_job(r, job);
    update_route(S->problem, r);
    S->ujobs[S->n_ujobs] = job;
    S->n_ujobs++;
}

// Snapshot of every currently-routed request as (route index, job pointer).
std::vector<std::pair<int, int*>> routed_requests(Solution* S) {
    std::vector<std::pair<int, int*>> routed;
    for (int t = 0; t < S->problem->M; t++) {
        Route* r = S->routes[t];
        for (int k = 0; k < r->njobs; k++) {
            routed.push_back({t, r->jobs[k]});
        }
    }
    return routed;
}

// --- Destroy operators --------------------------------------------------- //

// Random removal: q distinct requests drawn uniformly across all routes.
void destroy_random(Solution* S, double frac_min, double frac_max,
                     std::mt19937& rng) {
    int q = sample_removal_count(S, frac_min, frac_max, rng);
    for (int removed = 0; removed < q; removed++) {
        auto routed = routed_requests(S);
        if (routed.empty()) break;
        auto pick = routed[rng() % routed.size()];
        remove_request(S, S->routes[pick.first], pick.second);
    }
}

// Worst removal (Ropke & Pisinger Algorithm 3). Design decision 7: the true
// removal gain f(S) - f(S\{i}) is recomputed from scratch for every remaining
// candidate at every removal step (naive; see docs/ropke2006.md notes and
// GitHub issue #1 for the scaling follow-up once instances reach ~1000 nodes).
void destroy_worst(Solution* S, double p_worst, double frac_min,
                    double frac_max, const CostWeights& w,
                    std::mt19937& rng) {
    int q = sample_removal_count(S, frac_min, frac_max, rng);
    std::uniform_real_distribution<double> unif(0.0, 1.0);

    for (int removed = 0; removed < q; removed++) {
        auto routed = routed_requests(S);
        if (routed.empty()) break;

        double base_cost = cost(S, w);
        struct Candidate {
            int route_idx;
            int* job;
            double gain;
        };
        std::vector<Candidate> candidates;
        candidates.reserve(routed.size());
        for (auto& entry : routed) {
            Route* r = S->routes[entry.first];
            int* job = entry.second;
            int pos0 = find_node_position(r, job[0]);
            int pos1 = find_node_position(r, job[1]);
            remove_job(r, job);
            update_route(S->problem, r);
            double gain = base_cost - cost(S, w);
            insert_job(r, job, pos0, pos1);
            update_route(S->problem, r);
            candidates.push_back({entry.first, job, gain});
        }
        std::sort(candidates.begin(), candidates.end(),
                  [](const Candidate& a, const Candidate& b) {
                      return a.gain > b.gain;
                  });
        double y = std::pow(unif(rng), p_worst);
        int idx = std::min((int)(y * candidates.size()),
                            (int)candidates.size() - 1);
        remove_request(S, S->routes[candidates[idx].route_idx],
                        candidates[idx].job);
    }
}

// Shaw / relatedness removal (Ropke & Pisinger Algorithm 2). Ropke's fourth
// (vehicle-compatibility) relatedness term is dropped: this repo models a
// homogeneous fleet with no per-request vehicle subsets. The time-relatedness
// term uses each request's earliest time window (problem->e), not its
// realized service-begin time in the current solution, so this operator only
// needs the (static) problem data -- not a freshly updated route cache.
void destroy_shaw(Solution* S, double p_shaw, double phi, double chi,
                   double psi, double frac_min, double frac_max,
                   std::mt19937& rng) {
    int q = sample_removal_count(S, frac_min, frac_max, rng);
    if (q <= 0) return;
    PDPTW* P = S->problem;

    auto routed = routed_requests(S);
    if (routed.empty()) return;
    auto seed = routed[rng() % routed.size()];
    remove_request(S, S->routes[seed.first], seed.second);
    std::vector<int*> removed_jobs = {seed.second};

    for (int removed = 1; removed < q; removed++) {
        routed = routed_requests(S);
        if (routed.empty()) break;

        std::vector<std::pair<double, size_t>> ranked;
        ranked.reserve(routed.size());
        for (size_t c = 0; c < routed.size(); c++) {
            int* job = routed[c].second;
            double best_r = std::numeric_limits<double>::max();
            for (int* rj : removed_jobs) {
                double d = P->D[job[0]][rj[0]];
                double t = std::fabs(P->e[job[0]] - P->e[rj[0]]);
                double dem = std::fabs(P->q[job[0]] - P->q[rj[0]]);
                double rel = phi * d + chi * t + psi * dem;
                best_r = std::min(best_r, rel);
            }
            ranked.push_back({best_r, c});
        }
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<double, size_t>& a,
                     const std::pair<double, size_t>& b) {
                      return a.first < b.first;
                  });
        std::uniform_real_distribution<double> unif(0.0, 1.0);
        double y = std::pow(unif(rng), p_shaw);
        int idx = std::min((int)(y * ranked.size()), (int)ranked.size() - 1);
        auto chosen = routed[ranked[idx].second];
        remove_request(S, S->routes[chosen.first], chosen.second);
        removed_jobs.push_back(chosen.second);
    }
}

// --- Repair operators ----------------------------------------------------- //

struct InsertionOption {
    int route_idx;
    MoveCost mc;
};

// Best feasible insertion position for `job` in every one of the solution's
// M routes (routes are always pre-allocated up to M, see create_solution, so
// "open a new vehicle" is just the same insertion_cost call against whichever
// routes are still empty -- no special-casing needed). Infeasible routes
// (cost >= BIG_M) are excluded. Results are sorted ascending by cost.
std::vector<InsertionOption> rank_insertions(Solution* S, int* job, double mu,
                                              double alpha, bool apply_noise,
                                              double maxN,
                                              std::mt19937& rng) {
    std::vector<InsertionOption> options;
    std::uniform_real_distribution<double> noise_unif(-maxN, maxN);
    for (int t = 0; t < S->problem->M; t++) {
        MoveCost mc = insertion_cost(S->problem, S->routes[t], job, mu, alpha);
        if (mc.cost >= BIG_M) continue;
        if (apply_noise) {
            mc.cost = std::fmax(0.0, mc.cost + noise_unif(rng));
        }
        options.push_back({t, mc});
    }
    std::sort(options.begin(), options.end(),
              [](const InsertionOption& a, const InsertionOption& b) {
                  return a.mc.cost < b.mc.cost;
              });
    return options;
}

// Insert `mc.job` (== S->ujobs[job_idx]) into route `route_idx` at the
// position `mc` specifies, then compact it out of the unassigned-request
// bank -- mirrors insertion_step's bookkeeping (src/solution.c).
void commit_insertion(Solution* S, int route_idx, int job_idx,
                       const MoveCost& mc) {
    insert_job(S->routes[route_idx], mc.job, mc.pos[0], mc.pos[1]);
    update_route(S->problem, S->routes[route_idx]);
    for (int i = job_idx; i < S->n_ujobs - 1; i++) {
        S->ujobs[i] = S->ujobs[i + 1];
    }
    S->ujobs[S->n_ujobs - 1] = NULL;
    S->n_ujobs--;
}

// Basic greedy repair: insert the unassigned request with the single
// cheapest best-insertion cost, repeat until the bank is empty or nothing
// left in it has a feasible insertion anywhere.
void repair_greedy(Solution* S, double mu, double alpha, bool apply_noise,
                    double maxN, std::mt19937& rng) {
    while (S->n_ujobs > 0) {
        int best_job_idx = -1;
        int best_route = -1;
        MoveCost best{BIG_M, {0, 0}, NULL};
        for (int i = 0; i < S->n_ujobs; i++) {
            auto opts = rank_insertions(S, S->ujobs[i], mu, alpha, apply_noise,
                                        maxN, rng);
            if (opts.empty()) continue;
            if (opts[0].mc.cost < best.cost) {
                best = opts[0].mc;
                best_route = opts[0].route_idx;
                best_job_idx = i;
            }
        }
        if (best_job_idx < 0) break;
        commit_insertion(S, best_route, best_job_idx, best);
    }
}

// Regret-k repair (Ropke & Pisinger eq. 18-20): insert the unassigned request
// whose regret (sum of the cost gaps between its rank-1..rank-k insertion
// options) is largest, so requests that are about to run out of feasible
// slots get prioritized. Design decision 8: a request with fewer than k
// feasible options uses BIG_M for its missing ranks, matching the sentinel
// this codebase already uses everywhere else for "no feasible insertion".
void repair_regret_k(Solution* S, int k, double mu, double alpha,
                      bool apply_noise, double maxN, std::mt19937& rng) {
    while (S->n_ujobs > 0) {
        int best_job_idx = -1;
        int best_route = -1;
        MoveCost best_move{BIG_M, {0, 0}, NULL};
        double best_regret = -1.0;
        for (int i = 0; i < S->n_ujobs; i++) {
            auto opts = rank_insertions(S, S->ujobs[i], mu, alpha, apply_noise,
                                        maxN, rng);
            if (opts.empty()) continue;
            double regret = 0.0;
            for (int m = 1; m < k; m++) {
                double cm = (m < (int)opts.size()) ? opts[m].mc.cost
                                                    : (double)BIG_M;
                regret += cm - opts[0].mc.cost;
            }
            if (regret > best_regret) {
                best_regret = regret;
                best_job_idx = i;
                best_route = opts[0].route_idx;
                best_move = opts[0].mc;
            }
        }
        if (best_job_idx < 0) break;
        commit_insertion(S, best_route, best_job_idx, best_move);
    }
}

// --- Operator dispatch ---------------------------------------------------- //

struct AlnsParams {
    double mu;
    double alpha;
    double p_worst;
    double p_shaw;
    double shaw_phi;
    double shaw_chi;
    double shaw_psi;
    double removal_frac_min;
    double removal_frac_max;
    double maxN;  // noise_eta * max_ij(D_ij), computed once per run()
    CostWeights w;
};

void apply_destroy(int op, Solution* S, const AlnsParams& p,
                   std::mt19937& rng) {
    switch (op) {
        case D_RANDOM:
            destroy_random(S, p.removal_frac_min, p.removal_frac_max, rng);
            break;
        case D_WORST:
            destroy_worst(S, p.p_worst, p.removal_frac_min,
                         p.removal_frac_max, p.w, rng);
            break;
        case D_SHAW:
            destroy_shaw(S, p.p_shaw, p.shaw_phi, p.shaw_chi, p.shaw_psi,
                        p.removal_frac_min, p.removal_frac_max, rng);
            break;
    }
}

void apply_repair(int op, Solution* S, const AlnsParams& p, bool noise,
                  std::mt19937& rng) {
    switch (op) {
        case R_GREEDY:
            repair_greedy(S, p.mu, p.alpha, noise, p.maxN, rng);
            break;
        case R_REGRET2:
            repair_regret_k(S, 2, p.mu, p.alpha, noise, p.maxN, rng);
            break;
        case R_REGRET3:
            repair_regret_k(S, 3, p.mu, p.alpha, noise, p.maxN, rng);
            break;
    }
}

// Refresh every route's cached metrics (design decision 6: no touched-route
// tracking -- see GitHub issue #1 for the scaling follow-up).
void refresh_all_routes(Solution* S) {
    for (int t = 0; t < S->problem->M; t++) {
        update_route(S->problem, S->routes[t]);
    }
}

}  // namespace

// ===================================================================== //
//  Public interface                                                     //
// ===================================================================== //

alns::alns(int seg_size, double r, double sigma1, double sigma2,
          double sigma3, double p_worst, double p_shaw, double shaw_phi,
          double shaw_chi, double shaw_psi, double p_noise, double noise_eta,
          double w_unserved, double w_start_temp, double cooling_rate,
          double mu, double alpha, double w_nv, double w_dist, double w_sd,
          double w_wt, double removal_frac_min, double removal_frac_max,
          int max_iter, std::uint32_t seed)
    : m_seg_size(seg_size), m_r(r), m_sigma1(sigma1), m_sigma2(sigma2),
      m_sigma3(sigma3), m_p_worst(p_worst), m_p_shaw(p_shaw),
      m_shaw_phi(shaw_phi), m_shaw_chi(shaw_chi), m_shaw_psi(shaw_psi),
      m_p_noise(p_noise), m_noise_eta(noise_eta), m_w_unserved(w_unserved),
      m_w_start_temp(w_start_temp), m_cooling_rate(cooling_rate), m_mu(mu),
      m_alpha(alpha), m_wnv(w_nv), m_wdist(w_dist), m_wsd(w_sd), m_wwt(w_wt),
      m_removal_frac_min(removal_frac_min),
      m_removal_frac_max(removal_frac_max), m_max_iter(max_iter),
      m_seed(seed) {}

Solution* alns::run(Solution* initial) const {
    CostWeights w{m_wnv, m_wdist, m_wsd, m_wwt};
    std::mt19937 rng(m_seed);  // seeded from the constructor; --seed drives it

    PDPTW* P = initial->problem;
    double max_d = 0.0;
    for (int i = 0; i < P->N; i++) {
        for (int j = 0; j < P->N; j++) {
            max_d = std::fmax(max_d, P->D[i][j]);
        }
    }
    AlnsParams params{m_mu,       m_alpha,     m_p_worst,
                      m_p_shaw,   m_shaw_phi,  m_shaw_chi,
                      m_shaw_psi, m_removal_frac_min, m_removal_frac_max,
                      m_noise_eta * max_d, w};

    double z0 = cost(initial, w);
    // Design decision 5: Tstart such that a solution w_start_temp*100% worse
    // than z0 is accepted with probability 0.5 (Ropke & Pisinger Sec. 3.5).
    double T = (m_w_start_temp / std::log(2.0)) * z0;

    Solution* current = copy_solution(initial);
    Solution* best = copy_solution(initial);
    double best_cost = z0;
    double current_cost = eval_cost(current, w, m_w_unserved);

    std::array<double, 3> w_destroy{1.0, 1.0, 1.0};
    std::array<double, 3> w_repair{1.0, 1.0, 1.0};
    std::array<double, 3> score_destroy{0.0, 0.0, 0.0};
    std::array<double, 3> score_repair{0.0, 0.0, 0.0};
    std::array<int, 3> theta_destroy{0, 0, 0};
    std::array<int, 3> theta_repair{0, 0, 0};

    std::uniform_real_distribution<double> unif01(0.0, 1.0);
    std::bernoulli_distribution noise_flip(m_p_noise);

    for (int iter = 0; iter < m_max_iter; iter++) {
        Solution* trial = copy_solution(current);

        int d_idx = select_index(w_destroy, rng);
        int r_idx = select_index(w_repair, rng);
        bool noise_on = noise_flip(rng);

        apply_destroy(d_idx, trial, params, rng);
        refresh_all_routes(trial);
        apply_repair(r_idx, trial, params, noise_on, rng);
        refresh_all_routes(trial);

        double trial_cost = eval_cost(trial, w, m_w_unserved);

        double score = 0.0;
        if (trial->n_ujobs == 0 && trial_cost < best_cost) {
            free_solution(best);
            best = copy_solution(trial);
            best_cost = trial_cost;
            score = m_sigma1;
        } else if (trial_cost < current_cost) {
            score = m_sigma2;
        }

        bool accept = (trial_cost <= current_cost) ||
                      (unif01(rng) < std::exp(-(trial_cost - current_cost) / T));
        if (accept) {
            free_solution(current);
            current = trial;  // ownership transferred
            current_cost = trial_cost;
            if (score == 0.0) score = m_sigma3;
        } else {
            free_solution(trial);
        }

        score_destroy[d_idx] += score;
        theta_destroy[d_idx]++;
        score_repair[r_idx] += score;
        theta_repair[r_idx]++;

        T *= m_cooling_rate;

        if ((iter + 1) % m_seg_size == 0) {
            for (int i = 0; i < 3; i++) {
                if (theta_destroy[i] > 0) {
                    w_destroy[i] = w_destroy[i] * (1 - m_r) +
                                  m_r * score_destroy[i] / theta_destroy[i];
                }
                if (theta_repair[i] > 0) {
                    w_repair[i] = w_repair[i] * (1 - m_r) +
                                 m_r * score_repair[i] / theta_repair[i];
                }
                score_destroy[i] = 0.0;
                theta_destroy[i] = 0;
                score_repair[i] = 0.0;
                theta_repair[i] = 0;
            }
        }
    }

    free_solution(current);
    return best;
}

std::string alns::get_name() const {
    return "Adaptive Large Neighborhood Search";
}
