#pragma once

#include <cstdint>
#include <string>

// The solution data structures and PD operators are implemented in C.
extern "C" {
#include "solution.h"
}

/**
 * @class tabu_search
 * @brief Tabu-embedded simulated annealing metaheuristic for the PDPTW.
 *
 * Implements Section 3 of Li & Lim (2003), "A Metaheuristic for the Pickup and
 * Delivery Problem with Time Windows": a descent local search (DLS) over the
 * PD-Shift / PD-Exchange / PD-Rearrange neighborhoods, guided by a Metropolis
 * acceptance procedure with a tabu set of solution values, wrapped in a
 * K-restarts strategy.
 */
class tabu_search {
public:
    /// Default RNG seed: the shipped solver is deterministic unless reseeded.
    static constexpr std::uint32_t kDefaultSeed = 12345u;

    /**
     * @param K      Number of consecutive non-improving restarts before the
     *               overall algorithm stops (rightist K-ary tree depth).
     * @param MSNI   Max steps without improvement inside TABU_EMBEDDED_SA.
     * @param max_attempts  How many random neighbours METROPOLIS_PROC samples
     *               before giving up and leaving the incumbent unchanged. A
     *               neighbour is rejected without consuming the annealing
     *               temperature when it is tabu or structurally infeasible, so
     *               this bounds the hunt for a non-tabu move rather than the
     *               number of moves made.
     * @param T0     Initial annealing temperature, expressed as a fraction of
     *               the initial solution's SA cost (eq. 2): `Tstart =
     *               (T0 / ln 2) * sa_cost(initial)`, so a solution T0*100%
     *               worse than the start is accepted with probability 0.5.
     *               Mirrors how `alns`'s `w_start_temp` relates to its own
     *               starting cost `z0` — an absolute T0 does not transfer
     *               across instance sizes, since sa_cost grows with the
     *               product of two size-dependent quantities (distance and
     *               waiting time) while a fixed T0 does not.
     * @param delta  Cooling ratio applied on every accepted Metropolis move.
     * @param mu     Distance-saving weight of the insertion heuristic.
     * @param alpha  Distance/time trade-off of the insertion heuristic.
     * @param w_nv,w_dist,w_sd,w_wt  Penalty weights of the local-search cost
     *               function (eq. 1): alpha >> beta >> gamma >> lambda.
     * @param seed   Seed of the internal std::mt19937. Runs are deterministic
     *               given a seed; the default reproduces the historical
     *               hard-coded value, so omitting it leaves behavior unchanged.
     */
    explicit tabu_search(int K = 5, int MSNI = 3, int max_attempts = 2000,
                         double T0 = 0.05, double delta = 0.95,
                         double mu = 1.0, double alpha = 0.5,
                         double w_nv = 1e6, double w_dist = 1e3,
                         double w_sd = 1.0, double w_wt = 1e-3,
                         std::uint32_t seed = kDefaultSeed);

    /**
     * Run the overall algorithm starting from a feasible initial solution.
     *
     * @param initial A feasible solution (e.g. from insertion_heuristic). It is
     *                not modified or freed.
     * @return The best solution found; the caller owns it and must free it with
     *         free_solution.
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
    int m_K;
    int m_MSNI;
    int m_max_attempts;
    double m_T0;
    double m_delta;
    double m_mu;
    double m_alpha;
    double m_wnv;
    double m_wdist;
    double m_wsd;
    double m_wwt;
    std::uint32_t m_seed;
};
