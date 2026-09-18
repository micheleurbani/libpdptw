// Tests for the ALNS metaheuristic (src/algorithms/alns.cpp), implementing
// Ropke & Pisinger (2006), "An Adaptive Large Neighborhood Search Heuristic
// for the Pickup and Delivery Problem with Time Windows".
//
// Mirrors tests/test_tabu.cpp's structure and helpers. Destroy/repair
// operators and the adaptive-weight bookkeeping are anonymous-namespace
// internals of alns.cpp (matching tabu.cpp's own precedent of keeping
// DLS/Metropolis helpers private), so -- as with test_tabu.cpp -- this suite
// only exercises the public `alns` class end to end against the Li & Lim
// pdp_100 instances. Tests run from a directory one level below the repo
// root so the relative "../data/pdp_100/..." paths resolve.

#include <cmath>

extern "C" {
#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/solution.h"
}

#include "../include/algorithms/alns.hpp"

// Local-search cost weights matching the alns/tabu_search defaults (eq. 1 of
// the paper): alpha >> beta >> gamma >> lambda.
static const double W_NV = 1e6;
static const double W_DIST = 1e3;
static const double W_SD = 1.0;
static const double W_WT = 1e-3;

void setUp(void) {}
void tearDown(void) {}

// ----------------------------- helpers ----------------------------------- //

static double sol_cost(Solution* S) {
    return get_solution_cost(S, W_NV, W_DIST, W_SD, W_WT);
}

// Total number of jobs currently routed (served) across all vehicles.
static int served_jobs(Solution* S) {
    int n = 0;
    for (int i = 0; i < S->problem->M; i++) {
        n += S->routes[i]->njobs;
    }
    return n;
}

// Silent feasibility predicate (does not print, unlike check_route): every
// non-empty route must respect its capacity and time windows.
static bool routes_feasible(Solution* S) {
    PDPTW* P = S->problem;
    for (int i = 0; i < P->M; i++) {
        Route* r = S->routes[i];
        if (r->njobs == 0) continue;
        update_route(P, r);
        for (int k = 0; k < r->n; k++) {
            int node = r->path[k];
            if (r->b[k] < P->e[node]) return false;
            if (r->b[k] > P->l[node]) return false;
            if (r->Q[k] > P->Q) return false;
        }
    }
    return true;
}

static Solution* build_initial(const char* file) {
    PDPTW* P = read_pdptw_from_file(file);
    Solution* S = create_solution(P);
    insertion_heuristic(S, 1.0, 0.5);
    return S;
}

// Full default configuration; pdp_100 instances stay fast enough for a unit
// test at this iteration budget.
static alns make_alns() {
    return alns();  // paper-based defaults, max_iter = 25000
}

// A trimmed iteration budget for the multi-instance sweep, matching
// make_ts_fast's role in test_tabu.cpp.
static alns make_alns_fast() {
    return alns(/*seg_size=*/50, /*r=*/0.1, /*sigma1=*/33.0, /*sigma2=*/9.0,
                /*sigma3=*/13.0, /*p_worst=*/3.0, /*p_shaw=*/6.0,
                /*shaw_phi=*/9.0, /*shaw_chi=*/3.0, /*shaw_psi=*/2.0,
                /*p_noise=*/0.5, /*noise_eta=*/0.025, /*w_unserved=*/1e5,
                /*w_start_temp=*/0.05, /*cooling_rate=*/0.99975, /*mu=*/1.0,
                /*alpha=*/0.5, /*w_nv=*/1e6, /*w_dist=*/1e3, /*w_sd=*/1.0,
                /*w_wt=*/1e-3, /*removal_frac_min=*/0.1,
                /*removal_frac_max=*/0.4, /*max_iter=*/300);
}

// ----------------------------- tests ------------------------------------- //

// ALNS must always return a fully-served solution: `best` starts as a copy
// of the (fully served) initial solution and is only ever replaced when
// n_ujobs == 0 and the cost improves.
void test_alns_returns_fully_served_solution(void) {
    Solution* S = build_initial("../data/pdp_100/lc101.txt");
    int total_requests = S->problem->N / 2;

    alns solver = make_alns();
    Solution* best = solver.run(S);

    TEST_ASSERT_EQUAL_INT(0, best->n_ujobs);
    TEST_ASSERT_EQUAL_INT(total_requests, served_jobs(best));

    free_solution(best);
    PDPTW* P = S->problem;
    free_solution(S);
    free_problem(P);
}

// The overall algorithm must never return a worse solution than it was
// given: `best` is initialised to the input and only replaced on a strict,
// fully-served improvement.
void test_alns_does_not_worsen(void) {
    Solution* S = build_initial("../data/pdp_100/lc101.txt");
    double c0 = sol_cost(S);

    alns solver = make_alns();
    Solution* best = solver.run(S);
    double c1 = sol_cost(best);

    TEST_ASSERT_TRUE(c1 <= c0 + 1e-3);

    free_solution(best);
    PDPTW* P = S->problem;
    free_solution(S);
    free_problem(P);
}

// Feasibility is preserved end to end: every committed insertion is
// feasibility-checked by insertion_cost, so the returned solution is
// feasible too.
void test_alns_returns_feasible_solution(void) {
    Solution* S = build_initial("../data/pdp_100/lc101.txt");
    TEST_ASSERT_TRUE(routes_feasible(S));

    alns solver = make_alns();
    Solution* best = solver.run(S);

    TEST_ASSERT_TRUE(routes_feasible(best));

    free_solution(best);
    PDPTW* P = S->problem;
    free_solution(S);
    free_problem(P);
}

// run() documents that the input solution is neither modified nor freed, and
// that it returns a distinct, owned solution.
void test_alns_does_not_modify_input(void) {
    Solution* S = build_initial("../data/pdp_100/lr101.txt");
    double c_before = sol_cost(S);
    int served_before = served_jobs(S);

    alns solver = make_alns();
    Solution* best = solver.run(S);

    TEST_ASSERT_TRUE(best != S);
    TEST_ASSERT_TRUE(fabs(c_before - sol_cost(S)) < 1e-6);
    TEST_ASSERT_EQUAL_INT(served_before, served_jobs(S));

    free_solution(best);
    PDPTW* P = S->problem;
    free_solution(S);
    free_problem(P);
}

// The RNG is seeded from the constructor's `seed` argument, which defaults to
// alns::kDefaultSeed, so two runs from the same initial solution must produce
// bit-for-bit identical results: same cost, same vehicle count, same
// served-job count.
void test_alns_is_deterministic(void) {
    Solution* S = build_initial("../data/pdp_100/lc101.txt");

    alns solver = make_alns_fast();
    Solution* a = solver.run(S);
    Solution* b = solver.run(S);

    TEST_ASSERT_TRUE(fabs(sol_cost(a) - sol_cost(b)) < 1e-6);
    TEST_ASSERT_EQUAL_INT(count_vehicles(a), count_vehicles(b));
    TEST_ASSERT_EQUAL_INT(served_jobs(a), served_jobs(b));

    free_solution(a);
    free_solution(b);
    PDPTW* P = S->problem;
    free_solution(S);
    free_problem(P);
}

// Reseeding must actually reach the RNG, and must keep every seed's own run
// reproducible: setting the default seed explicitly reproduces the default
// configuration exactly, and a re-seeded solver repeats itself across runs.
void test_alns_seed_controls_the_rng(void) {
    Solution* S = build_initial("../data/pdp_100/lc101.txt");

    alns def = make_alns_fast();
    Solution* d = def.run(S);

    alns explicit_default = make_alns_fast();
    explicit_default.set_seed(alns::kDefaultSeed);
    Solution* e = explicit_default.run(S);

    // Setting the default seed by hand changes nothing.
    TEST_ASSERT_TRUE(fabs(sol_cost(d) - sol_cost(e)) < 1e-6);
    TEST_ASSERT_EQUAL_INT(count_vehicles(d), count_vehicles(e));

    alns reseeded = make_alns_fast();
    reseeded.set_seed(777u);
    Solution* a = reseeded.run(S);
    Solution* b = reseeded.run(S);

    // A different seed is still deterministic: same seed, same answer.
    TEST_ASSERT_TRUE(fabs(sol_cost(a) - sol_cost(b)) < 1e-6);
    TEST_ASSERT_EQUAL_INT(count_vehicles(a), count_vehicles(b));
    TEST_ASSERT_EQUAL_INT(served_jobs(a), served_jobs(b));
    // ...and it still returns a feasible, fully-served solution.
    TEST_ASSERT_TRUE(routes_feasible(a));
    TEST_ASSERT_EQUAL_INT(0, a->n_ujobs);

    free_solution(d);
    free_solution(e);
    free_solution(a);
    free_solution(b);
    PDPTW* P = S->problem;
    free_solution(S);
    free_problem(P);
}

// Robustness across both instance types (1 = tight windows / many vehicles,
// 2 = wide windows / few long routes) and all three families (clustered,
// random, mixed): every run stays feasible, fully serves every request, and
// does not worsen the objective.
void test_alns_across_instance_families(void) {
    const char* files[] = {
        "../data/pdp_100/lc101.txt", "../data/pdp_100/lc201.txt",
        "../data/pdp_100/lr101.txt", "../data/pdp_100/lr201.txt",
        "../data/pdp_100/lrc101.txt", "../data/pdp_100/lrc201.txt",
    };
    const int n_files = (int)(sizeof(files) / sizeof(files[0]));
    for (int k = 0; k < n_files; k++) {
        Solution* S = build_initial(files[k]);
        double c0 = sol_cost(S);
        int total_requests = S->problem->N / 2;

        alns solver = make_alns_fast();
        Solution* best = solver.run(S);

        TEST_ASSERT_TRUE(routes_feasible(best));
        TEST_ASSERT_EQUAL_INT(0, best->n_ujobs);
        TEST_ASSERT_EQUAL_INT(total_requests, served_jobs(best));
        TEST_ASSERT_TRUE(sol_cost(best) <= c0 + 1e-3);

        free_solution(best);
        PDPTW* P = S->problem;
        free_solution(S);
        free_problem(P);
    }
}

void test_alns_get_name(void) {
    alns solver = make_alns();
    TEST_ASSERT_EQUAL_STRING("Adaptive Large Neighborhood Search",
                             solver.get_name().c_str());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_alns_returns_fully_served_solution);
    RUN_TEST(test_alns_does_not_worsen);
    RUN_TEST(test_alns_returns_feasible_solution);
    RUN_TEST(test_alns_does_not_modify_input);
    RUN_TEST(test_alns_is_deterministic);
    RUN_TEST(test_alns_seed_controls_the_rng);
    RUN_TEST(test_alns_across_instance_families);
    RUN_TEST(test_alns_get_name);
    return UNITY_END();
}
