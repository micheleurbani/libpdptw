#include "algorithms/tabu.hpp"

#include <climits>
#include <cmath>
#include <random>
#include <set>
#include <tuple>
#include <vector>

extern "C" {
#include "config.h"
}

// ===================================================================== //
//  Internal helpers (file-local)                                        //
// ===================================================================== //
namespace {

// Penalty weights of the local-search cost function (eq. 1 of the paper).
struct Weights {
    double nv;
    double dist;
    double sd;
    double wt;
};

// SolutionValue (paper Section 3.1): (NV, TC, SD, WT). Two solutions are
// considered identical for the tabu set if they share this 4-tuple. The real
// components are quantized so equality is stable against floating-point noise.
struct SolutionValue {
    long nv;
    long tc;
    long sd;
    long wt;
    bool operator<(const SolutionValue& o) const {
        return std::tie(nv, tc, sd, wt) < std::tie(o.nv, o.tc, o.sd, o.wt);
    }
};

using TabuSet = std::set<SolutionValue>;

long quantize(double v) { return std::llround(v * 1e6); }

SolutionValue make_value(Solution* S) {
    SolutionValue v;
    v.nv = count_vehicles(S);
    v.tc = quantize(get_total_travel_cost(S));
    v.sd = quantize(get_total_schedule_duration(S));
    v.wt = quantize(get_total_waiting_time(S));
    return v;
}

// Priority-ordered objective used for descent local search (eq. 1).
double cost(Solution* S, const Weights& w) {
    return get_solution_cost(S, w.nv, w.dist, w.sd, w.wt);
}

// Simulated-annealing cost (eq. 2): SACost(S) = Dist(S) + phi * WT(S),
// with phi = 0.01 * Dist(S).
double sa_cost(Solution* S) {
    double dist = get_total_travel_cost(S);
    double wt = get_total_waiting_time(S);
    double phi = 0.01 * dist;
    return dist + phi * wt;
}

// --- Neighbor generation: clone, apply one operator, refresh both routes. ---
// Each returns a newly owned solution, or nullptr if the move is infeasible.

Solution* try_shift(Solution* S, int i, int j, double mu, double alpha) {
    Solution* c = copy_solution(S);
    if (!pd_shift(c, c->routes[i], c->routes[j], mu, alpha)) {
        free_solution(c);
        return nullptr;
    }
    // pd_shift does not refresh the route metrics; do it here for both routes.
    update_route(c->problem, c->routes[i]);
    update_route(c->problem, c->routes[j]);
    return c;
}

Solution* try_exchange(Solution* S, int i, int j, double mu, double alpha) {
    Solution* c = copy_solution(S);
    if (!pd_exchange(c, c->routes[i], c->routes[j], mu, alpha)) {
        free_solution(c);
        return nullptr;
    }
    update_route(c->problem, c->routes[i]);
    update_route(c->problem, c->routes[j]);
    return c;
}

Solution* try_rearrange(Solution* S, int i, double mu, double alpha) {
    Solution* c = copy_solution(S);
    if (!pd_rearrange(c, c->routes[i], mu, alpha)) {
        free_solution(c);
        return nullptr;
    }
    update_route(c->problem, c->routes[i]);
    return c;
}

// DLS over N_PDS U N_PDE: repeatedly adopt the best improving Shift/Exchange
// neighbor until no neighbor lowers the objective.
Solution* dls_shift_exchange(Solution* S, const Weights& w,
                             double mu, double alpha) {
    Solution* best = copy_solution(S);
    double best_cost = cost(best, w);
    int M = best->problem->M;
    bool improved = true;
    while (improved) {
        improved = false;
        Solution* best_neighbor = nullptr;
        double best_neighbor_cost = best_cost;
        for (int i = 0; i < M; i++) {
            if (best->routes[i]->njobs == 0) continue;
            for (int j = 0; j < M; j++) {
                if (i == j || best->routes[j]->njobs == 0) continue;
                // PD-Shift is directional: evaluate i -> j for every ordered pair.
                if (Solution* c = try_shift(best, i, j, mu, alpha)) {
                    double cc = cost(c, w);
                    if (cc < best_neighbor_cost) {
                        if (best_neighbor) free_solution(best_neighbor);
                        best_neighbor = c;
                        best_neighbor_cost = cc;
                    } else {
                        free_solution(c);
                    }
                }
                // PD-Exchange is symmetric: evaluate each unordered pair once.
                if (j > i) {
                    if (Solution* e = try_exchange(best, i, j, mu, alpha)) {
                        double ec = cost(e, w);
                        if (ec < best_neighbor_cost) {
                            if (best_neighbor) free_solution(best_neighbor);
                            best_neighbor = e;
                            best_neighbor_cost = ec;
                        } else {
                            free_solution(e);
                        }
                    }
                }
            }
        }
        if (best_neighbor) {
            free_solution(best);
            best = best_neighbor;
            best_cost = best_neighbor_cost;
            improved = true;
        }
    }
    return best;
}

// DLS over N_PDR (post-optimization within each route).
Solution* dls_rearrange(Solution* S, const Weights& w,
                        double mu, double alpha) {
    Solution* best = copy_solution(S);
    double best_cost = cost(best, w);
    int M = best->problem->M;
    bool improved = true;
    while (improved) {
        improved = false;
        Solution* best_neighbor = nullptr;
        double best_neighbor_cost = best_cost;
        for (int i = 0; i < M; i++) {
            if (best->routes[i]->njobs == 0) continue;
            if (Solution* c = try_rearrange(best, i, mu, alpha)) {
                double cc = cost(c, w);
                if (cc < best_neighbor_cost) {
                    if (best_neighbor) free_solution(best_neighbor);
                    best_neighbor = c;
                    best_neighbor_cost = cc;
                } else {
                    free_solution(c);
                }
            }
        }
        if (best_neighbor) {
            free_solution(best);
            best = best_neighbor;
            best_cost = best_neighbor_cost;
            improved = true;
        }
    }
    return best;
}

// Relocate every PD-pair of route `src` into the cheapest feasible *non-empty*
// route, so that `src` ends up empty (one fewer vehicle). Mutates S in place;
// returns false (leaving S partially modified) if some pair cannot be moved.
bool empty_route(Solution* S, int src, double mu, double alpha) {
    Route* r = S->routes[src];
    int M = S->problem->M;
    while (r->njobs > 0) {
        int* job = r->jobs[0];
        MoveCost best = {BIG_M, {0, 0}, NULL};
        int best_t = -1;
        for (int t = 0; t < M; t++) {
            if (t == src || S->routes[t]->njobs == 0) continue;
            MoveCost c = insertion_cost(S->problem, S->routes[t], job, mu, alpha);
            if (c.cost < best.cost) {
                best = c;
                best_t = t;
            }
        }
        if (best_t < 0 || best.cost >= BIG_M) return false;
        remove_job(r, job);
        update_route(S->problem, r);
        insert_job(S->routes[best_t], best.job, best.pos[0], best.pos[1]);
        update_route(S->problem, S->routes[best_t]);
    }
    return true;
}

// Step 5.2: reduce the number of vehicles by repeatedly emptying the smallest
// non-empty route (relocating its pairs at minimal cost increment). Returns a
// newly owned solution; the input is left untouched.
Solution* reduce_vehicles(Solution* S, double mu, double alpha) {
    Solution* cur = copy_solution(S);
    while (true) {
        int src = -1;
        int min_jobs = INT_MAX;
        for (int i = 0; i < cur->problem->M; i++) {
            int nj = cur->routes[i]->njobs;
            if (nj > 0 && nj < min_jobs) {
                min_jobs = nj;
                src = i;
            }
        }
        if (src < 0) break;
        Solution* trial = copy_solution(cur);
        if (empty_route(trial, src, mu, alpha)) {
            free_solution(cur);
            cur = trial;  // one vehicle removed; try to reduce further
        } else {
            free_solution(trial);
            break;  // smallest route cannot be emptied; stop
        }
    }
    return cur;
}

// Outcome of the Metropolis procedure: the accepted solution plus the two
// routes touched by the Shift/Exchange move (for the step-5.3 re-ordering).
struct MetropolisResult {
    Solution* S;
    int r1;
    int r2;
};

// METROPOLIS_PROC: draw random Shift/Exchange neighbors (not in the tabu set)
// until one is accepted by the Metropolis criterion on SACost. On acceptance
// the global temperature is cooled and the solution value is recorded as tabu.
MetropolisResult metropolis_proc(Solution* S, double& T, double delta,
                                 TabuSet& tabu, std::mt19937& rng,
                                 double mu, double alpha, int max_attempts) {
    std::uniform_real_distribution<double> unif(0.0, 1.0);
    int M = S->problem->M;
    double base = sa_cost(S);

    std::vector<int> nonempty;
    for (int i = 0; i < M; i++) {
        if (S->routes[i]->njobs > 0) nonempty.push_back(i);
    }
    if (nonempty.empty() || M < 2) {
        return {copy_solution(S), -1, -1};
    }

    for (int attempt = 0; attempt < max_attempts; attempt++) {
        int a = nonempty[rng() % nonempty.size()];
        // Prefer an exchange when a second non-empty route exists; otherwise a
        // shift (whose target may be an empty route, spawning a new vehicle).
        bool can_exchange = nonempty.size() > 1;
        bool use_exchange = can_exchange && (rng() % 2 == 0);

        Solution* cand = nullptr;
        int r1 = a, r2 = -1;
        if (use_exchange) {
            do {
                r2 = nonempty[rng() % nonempty.size()];
            } while (r2 == a);
            cand = try_exchange(S, a, r2, mu, alpha);
        } else {
            do {
                r2 = rng() % M;
            } while (r2 == a);
            cand = try_shift(S, a, r2, mu, alpha);
        }
        if (!cand) continue;

        SolutionValue v = make_value(cand);
        if (tabu.count(v)) {
            free_solution(cand);
            continue;
        }

        double delta_e = sa_cost(cand) - base;
        double prob = (delta_e <= 0.0) ? 1.0 : std::exp(-delta_e / T);
        if (unif(rng) <= prob) {
            T *= delta;
            tabu.insert(v);
            return {cand, r1, r2};
        }
        free_solution(cand);
    }
    // No neighbor accepted within the attempt budget: keep S unchanged.
    return {copy_solution(S), -1, -1};
}

// TABU_EMBEDDED_SA(S): one tabu-guided simulated-annealing pass.
Solution* tabu_embedded_sa(Solution* S_in, const Weights& w, double& T,
                           double delta, int MSNI, TabuSet& tabu,
                           std::mt19937& rng, double mu, double alpha,
                           int max_attempts) {
    // 1-2. Sb <- DLS over N_PDS U N_PDE, then DLS over N_PDR.
    Solution* Sb = dls_shift_exchange(S_in, w, mu, alpha);
    Solution* tmp = dls_rearrange(Sb, w, mu, alpha);
    free_solution(Sb);
    Sb = tmp;

    int NoImpr = 0;
    Solution* S = copy_solution(Sb);

    while (NoImpr < MSNI) {
        // 5.1 Random accepted neighbor from the Shift/Exchange neighborhood.
        MetropolisResult mr =
            metropolis_proc(S, T, delta, tabu, rng, mu, alpha, max_attempts);
        Solution* Sp = mr.S;

        // 5.2 Try to remove a vehicle.
        Solution* reduced = reduce_vehicles(Sp, mu, alpha);
        free_solution(Sp);
        Sp = reduced;

        // 5.3 Re-order the two routes touched by the Metropolis move.
        if (mr.r1 >= 0) reorder_route(Sp, mr.r1, mu, alpha);
        if (mr.r2 >= 0) reorder_route(Sp, mr.r2, mu, alpha);

        // 5.4-5.5 DLS over N_PDS U N_PDE, then over N_PDR.
        Solution* Sbp = dls_shift_exchange(Sp, w, mu, alpha);
        free_solution(Sp);
        Solution* Sbp2 = dls_rearrange(Sbp, w, mu, alpha);
        free_solution(Sbp);
        Sbp = Sbp2;

        // 5.6-5.7 Track improvement against the incumbent.
        if (cost(Sbp, w) < cost(Sb, w)) {
            free_solution(Sb);
            Sb = copy_solution(Sbp);
            NoImpr = 0;
        } else {
            NoImpr++;
        }

        // 5.8 S <- Sb'.
        free_solution(S);
        S = Sbp;  // ownership transferred
    }

    free_solution(S);
    return Sb;
}

}  // namespace

// ===================================================================== //
//  Public interface                                                     //
// ===================================================================== //

tabu_search::tabu_search(int K, int MSNI, int max_attempts, double T0,
                         double delta, double mu, double alpha, double w_nv,
                         double w_dist, double w_sd, double w_wt,
                         std::uint32_t seed)
    : m_K(K), m_MSNI(MSNI), m_max_attempts(max_attempts), m_T0(T0),
      m_delta(delta), m_mu(mu), m_alpha(alpha),
      m_wnv(w_nv), m_wdist(w_dist), m_wsd(w_sd), m_wwt(w_wt), m_seed(seed) {}

Solution* tabu_search::run(Solution* initial) const {
    Weights w{m_wnv, m_wdist, m_wsd, m_wwt};
    std::mt19937 rng(m_seed);
    TabuSet tabu;
    // T0 is a fraction of the initial solution's SA cost, not an absolute
    // temperature (see tabu_search's Doxygen comment): a solution T0*100%
    // worse than sa_cost(initial) is accepted with probability 0.5, mirroring
    // alns's Tstart = (w_start_temp / ln 2) * z0.
    double T = (m_T0 / std::log(2.0)) * sa_cost(initial);

    // OVERALL_ALGORITHMS: K-restarts around TABU_EMBEDDED_SA.
    Solution* Sb = copy_solution(initial);
    int gNoImpro = 0;
    while (gNoImpro < m_K) {
        Solution* S = copy_solution(Sb);
        Solution* Sbp = tabu_embedded_sa(S, w, T, m_delta, m_MSNI, tabu, rng,
                                         m_mu, m_alpha, m_max_attempts);
        free_solution(S);
        if (cost(Sbp, w) < cost(Sb, w)) {
            free_solution(Sb);
            Sb = Sbp;
            gNoImpro = 0;
        } else {
            free_solution(Sbp);
            gNoImpro++;
        }
    }
    return Sb;
}

std::string tabu_search::get_name() const {
    return "Tabu-Embedded Simulated Annealing";
}
