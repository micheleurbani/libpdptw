#pragma once

#include <cstdint>
#include <string>

// The solution data structures and PD operators are implemented in C.
extern "C" {
#include "solution.h"
}

/**
 * @class alns
 * @brief Adaptive Large Neighborhood Search metaheuristic for the PDPTW.
 *
 * Implements Ropke & Pisinger (2006), "An Adaptive Large Neighborhood Search
 * Heuristic for the Pickup and Delivery Problem with Time Windows": each
 * iteration removes a random-sized batch of requests from the solution (a
 * destroy operator), reinserts them (a repair operator), and accepts or
 * rejects the result via simulated annealing. Destroy and repair operators
 * are chosen by roulette wheel from a pool, with selection weights adapted
 * every `seg_size` iterations based on each operator's track record.
 *
 * Unlike `tabu_search`, ALNS has no notion of infeasible-but-unserved
 * requests being disallowed mid-search: a destroy+repair cycle may leave some
 * requests unassigned (`Solution::ujobs`). Such trial solutions are scored
 * with a penalized cost (`get_solution_cost(...) + w_unserved * n_ujobs`) for
 * the SA acceptance test, but they can never become the returned "best"
 * solution, which is only ever updated when every request is served.
 */
class alns {
public:
    /// Default RNG seed: the shipped solver is deterministic unless reseeded.
    static constexpr std::uint32_t kDefaultSeed = 12345u;

    /**
     * @param seg_size   Iterations per adaptive-weight segment.
     * @param r          Reaction factor: how fast weights move toward the
     *                   latest segment's scores (eq. w_{j+1} = w_j(1-r) + r*s/theta).
     * @param sigma1     Score awarded when an iteration yields a new global best.
     * @param sigma2     Score awarded when an iteration improves the current
     *                   (not necessarily best) solution.
     * @param sigma3     Score awarded when a worse solution is accepted by SA.
     * @param p_worst    Determinism exponent of worst removal (Algorithm 3):
     *                   higher values pick the worst candidate more often.
     * @param p_shaw     Determinism exponent of Shaw/relatedness removal
     *                   (Algorithm 2).
     * @param shaw_phi   Distance weight in the Shaw relatedness measure.
     * @param shaw_chi   Time-window weight in the Shaw relatedness measure.
     * @param shaw_psi   Demand weight in the Shaw relatedness measure.
     * @param p_noise    Probability that insertion-cost noise (Sec. 3.6) is
     *                   applied during a given iteration's repair pass.
     * @param noise_eta  Noise magnitude control: maxN = noise_eta * max_ij(D_ij).
     * @param w_unserved Penalty per unassigned request added to
     *                   get_solution_cost for the SA acceptance test.
     * @param w_start_temp  Start-temperature control parameter (Sec. 3.5):
     *                   Tstart = (w_start_temp / ln 2) * z0, so a solution
     *                   w_start_temp*100% worse than the initial solution is
     *                   accepted with probability 0.5 at the start.
     * @param cooling_rate  Geometric cooling ratio applied every iteration.
     * @param mu         Distance-saving weight of the insertion heuristic.
     * @param alpha      Distance/time trade-off of the insertion heuristic.
     * @param w_nv,w_dist,w_sd,w_wt  Penalty weights of the local-search cost
     *                   function (eq. 1): NV >> TC >> SD >> WT.
     * @param removal_frac_min  Minimum fraction of requests removed per destroy call.
     * @param removal_frac_max  Maximum fraction of requests removed per destroy call.
     * @param max_iter   Total number of ALNS iterations to run.
     * @param seed       Seed of the internal std::mt19937. Runs are
     *                   deterministic given a seed; the default reproduces the
     *                   historical hard-coded value, so omitting it leaves
     *                   behavior unchanged.
     */
    explicit alns(int seg_size = 100,
                  double r = 0.1,
                  double sigma1 = 33.0,
                  double sigma2 = 9.0,
                  double sigma3 = 13.0,
                  double p_worst = 3.0,
                  double p_shaw = 6.0,
                  double shaw_phi = 9.0,
                  double shaw_chi = 3.0,
                  double shaw_psi = 2.0,
                  double p_noise = 0.5,
                  double noise_eta = 0.025,
                  double w_unserved = 1e5,
                  double w_start_temp = 0.05,
                  double cooling_rate = 0.99975,
                  double mu = 1.0,
                  double alpha = 0.5,
                  double w_nv = 1e6,
                  double w_dist = 1e3,
                  double w_sd = 1.0,
                  double w_wt = 1e-3,
                  double removal_frac_min = 0.1,
                  double removal_frac_max = 0.4,
                  int max_iter = 25000,
                  std::uint32_t seed = kDefaultSeed);

    /**
     * Run the overall algorithm starting from a feasible initial solution.
     *
     * @param initial A feasible solution (e.g. from insertion_heuristic). It is
     *                not modified or freed.
     * @return The best fully-served solution found; the caller owns it and
     *         must free it with free_solution.
     */
    Solution* run(Solution* initial) const;

    /**
     * Reseed the internal RNG. Provided so a caller that keeps every other
     * tunable at its default (e.g. the `--seed` flag of `pdptw_solver`) need
     * not respell the whole constructor argument list.
     */
    void set_seed(std::uint32_t seed) { m_seed = seed; }

    std::string get_name() const;

private:
    int m_seg_size;
    double m_r;
    double m_sigma1;
    double m_sigma2;
    double m_sigma3;
    double m_p_worst;
    double m_p_shaw;
    double m_shaw_phi;
    double m_shaw_chi;
    double m_shaw_psi;
    double m_p_noise;
    double m_noise_eta;
    double m_w_unserved;
    double m_w_start_temp;
    double m_cooling_rate;
    double m_mu;
    double m_alpha;
    double m_wnv;
    double m_wdist;
    double m_wsd;
    double m_wwt;
    double m_removal_frac_min;
    double m_removal_frac_max;
    int m_max_iter;
    std::uint32_t m_seed;
};
