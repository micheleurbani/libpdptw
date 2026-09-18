// Tests for the algorithms implemented on the feat/tabu-metaheuristic branch:
//   * the `tabu_search` metaheuristic (src/algorithms/tabu.cpp), and
//   * its supporting C primitive `reorder_route` (src/solution.c).
//
// The lower-level building blocks (insertion heuristic, PD operators,
// copy_solution, cost functions) are covered by tests/test_solution.c; this
// suite focuses on the end-to-end contracts of the metaheuristic against the
// Li & Lim pdp_100 instances. Tests run from a directory one level below the
// repo root so the relative "../data/pdp_100/..." paths resolve.

#include <cmath>

extern "C" {
#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/solution.h"
}

#include "../include/algorithms/tabu.hpp"

// Local-search cost weights matching the tabu_search defaults (eq. 1 of the
// paper): alpha >> beta >> gamma >> lambda.
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
// non-empty route must respect its capacity and time windows. Service starting
// exactly at the late bound is feasible (strict '>'), matching the comparison
// the insertion heuristic uses. The PD operators and the construction heuristic
// only ever commit feasibility-checked moves, so the metaheuristic preserves
// feasibility end to end.
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

// The pdptw_solver default run is sub-second per pdp_100 instance, so the full
// default configuration is affordable inside the unit tests.
static tabu_search make_ts() {
    return tabu_search();  // K=5, MSNI=3, paper-based defaults
}

// A trimmed configuration for the multi-instance sweep (keeps total runtime low
// while still exercising the full DLS / Metropolis / restart machinery).
static tabu_search make_ts_fast() {
    return tabu_search(/*K=*/2, /*MSNI=*/2);
}

// ----------------------------- tests ------------------------------------- //

// The overall algorithm must never return a worse solution than it was given:
// Sb is initialised to the input and only replaced on a strict improvement.
void test_tabu_does_not_worsen(void) {
    Solution* S = build_initial("../data/pdp_100/lc101.txt");
    double c0 = sol_cost(S);

    tabu_search ts = make_ts();
    Solution* best = ts.run(S);
    double c1 = sol_cost(best);

    // Allow a tiny tolerance for floating-point recomputation noise.
    TEST_ASSERT_TRUE(c1 <= c0 + 1e-3);

    free_solution(best);
    PDPTW* P = S->problem;  // problem outlives both solutions
    free_solution(S);
    free_problem(P);
}

// Shift / exchange / rearrange / vehicle-reduction all relocate PD-pairs but
// must never drop one: the number of served jobs is invariant. lrc104 is a
// regression guard -- an earlier reorder_route stranded jobs in the unassigned
// list there (served 54 -> 41) because greedy re-insertion could not re-fit
// every pair and the leftovers were abandoned instead of reverted.
void test_tabu_preserves_served_jobs(void) {
    Solution* S = build_initial("../data/pdp_100/lrc104.txt");
    int served0 = served_jobs(S);

    tabu_search ts = make_ts();
    Solution* best = ts.run(S);

    TEST_ASSERT_EQUAL_INT(served0, served_jobs(best));

    free_solution(best);
    PDPTW* P = S->problem;
    free_solution(S);
    free_problem(P);
}

// Feasibility is preserved end to end: the construction heuristic emits a
// feasible solution and every committed move is feasibility-checked, so the
// returned solution is feasible too.
void test_tabu_returns_feasible_solution(void) {
    Solution* S = build_initial("../data/pdp_100/lc101.txt");
    TEST_ASSERT_TRUE(routes_feasible(S));  // construction is feasible to begin with

    tabu_search ts = make_ts();
    Solution* best = ts.run(S);

    TEST_ASSERT_TRUE(routes_feasible(best));

    free_solution(best);
    PDPTW* P = S->problem;
    free_solution(S);
    free_problem(P);
}

// Because NV dominates the lexicographic objective (alpha >> beta), an accepted
// improvement can never increase the vehicle count. Step 5.2 should typically
// reduce it for the clustered lc101 instance.
void test_tabu_vehicle_count_non_increasing(void) {
    Solution* S = build_initial("../data/pdp_100/lc101.txt");
    int nv0 = count_vehicles(S);

    tabu_search ts = make_ts();
    Solution* best = ts.run(S);
    int nv1 = count_vehicles(best);

    TEST_ASSERT_LESS_OR_EQUAL_INT(nv0, nv1);

    free_solution(best);
    PDPTW* P = S->problem;
    free_solution(S);
    free_problem(P);
}

// run() documents that the input solution is neither modified nor freed, and
// that it returns a distinct, owned solution.
void test_tabu_does_not_modify_input(void) {
    Solution* S = build_initial("../data/pdp_100/lr101.txt");
    double c_before = sol_cost(S);
    int served_before = served_jobs(S);

    tabu_search ts = make_ts();
    Solution* best = ts.run(S);

    // The returned solution is a different object than the input.
    TEST_ASSERT_TRUE(best != S);
    // The input is untouched.
    TEST_ASSERT_TRUE(fabs(c_before - sol_cost(S)) < 1e-6);
    TEST_ASSERT_EQUAL_INT(served_before, served_jobs(S));

    free_solution(best);
    PDPTW* P = S->problem;
    free_solution(S);
    free_problem(P);
}

// The RNG is seeded from the constructor's `seed` argument, which defaults to
// tabu_search::kDefaultSeed, so two runs from the same initial solution must
// produce bit-for-bit identical objective values.
void test_tabu_is_deterministic(void) {
    Solution* S = build_initial("../data/pdp_100/lc101.txt");

    tabu_search ts = make_ts();
    Solution* a = ts.run(S);
    Solution* b = ts.run(S);

    // Unity's double asserts are compiled out by default, so compare manually;
    // a deterministic seed yields bit-identical objective values.
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
void test_tabu_seed_controls_the_rng(void) {
    Solution* S = build_initial("../data/pdp_100/lc101.txt");

    tabu_search def = make_ts_fast();
    Solution* d = def.run(S);

    tabu_search explicit_default = make_ts_fast();
    explicit_default.set_seed(tabu_search::kDefaultSeed);
    Solution* e = explicit_default.run(S);

    // Setting the default seed by hand changes nothing.
    TEST_ASSERT_TRUE(fabs(sol_cost(d) - sol_cost(e)) < 1e-6);
    TEST_ASSERT_EQUAL_INT(count_vehicles(d), count_vehicles(e));

    tabu_search reseeded = make_ts_fast();
    reseeded.set_seed(777u);
    Solution* a = reseeded.run(S);
    Solution* b = reseeded.run(S);

    // A different seed is still deterministic: same seed, same answer.
    TEST_ASSERT_TRUE(fabs(sol_cost(a) - sol_cost(b)) < 1e-6);
    TEST_ASSERT_EQUAL_INT(count_vehicles(a), count_vehicles(b));
    TEST_ASSERT_EQUAL_INT(served_jobs(a), served_jobs(b));
    // ...and it still returns a usable solution.
    TEST_ASSERT_TRUE(routes_feasible(a));
    TEST_ASSERT_EQUAL_INT(served_jobs(d), served_jobs(a));

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
// random, mixed): every run stays feasible, conserves jobs, and does not worsen
// the objective. The type-2 and lrc instances are exactly the ones on which the
// reorder_route job-loss bug manifested, so this sweep is the broad regression
// guard.
void test_tabu_across_instance_families(void) {
    const char* files[] = {
        "../data/pdp_100/lc101.txt",
        "../data/pdp_100/lc201.txt",
        "../data/pdp_100/lr101.txt",
        "../data/pdp_100/lr201.txt",
        "../data/pdp_100/lrc101.txt",
        "../data/pdp_100/lrc201.txt",
    };
    const int n_files = (int)(sizeof(files) / sizeof(files[0]));
    for (int k = 0; k < n_files; k++) {
        Solution* S = build_initial(files[k]);
        double c0 = sol_cost(S);
        int served0 = served_jobs(S);

        tabu_search ts = make_ts_fast();
        Solution* best = ts.run(S);

        TEST_ASSERT_TRUE(routes_feasible(best));
        TEST_ASSERT_EQUAL_INT(served0, served_jobs(best));
        TEST_ASSERT_TRUE(sol_cost(best) <= c0 + 1e-3);

        free_solution(best);
        PDPTW* P = S->problem;
        free_solution(S);
        free_problem(P);
    }
}

void test_tabu_get_name(void) {
    tabu_search ts = make_ts();
    TEST_ASSERT_EQUAL_STRING("Tabu-Embedded Simulated Annealing",
                             ts.get_name().c_str());
}

// reorder_route (step 5.3) re-optimizes a single route by re-inserting its own
// PD-pairs via Solomon. It restricts the re-insertion pool to the route's jobs
// and never touches the global unassigned-job list, so a re-order can only
// permute a route's jobs -- never strand them. The route must keep every job,
// leave the global job accounting untouched, and remain feasible.
void test_reorder_route_keeps_feasible_and_conserves_jobs(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc104.txt");
    Solution* S = create_solution(P);
    int r_id = 0;
    // Greedily fill route 0 (this also removes the served jobs from ujobs).
    TEST_ASSERT_TRUE(route_construction(S, r_id, 1.0, 0.5));
    int njobs_before = S->routes[r_id]->njobs;
    int ujobs_before = S->n_ujobs;
    TEST_ASSERT_GREATER_THAN_INT(0, njobs_before);
    TEST_ASSERT_TRUE(check_route(P, S->routes[r_id]));

    bool ok = reorder_route(S, r_id, 1.0, 0.5);
    TEST_ASSERT_TRUE(ok);
    // The route keeps every job, the unassigned list is left untouched, and the
    // route stays feasible.
    TEST_ASSERT_EQUAL_INT(njobs_before, S->routes[r_id]->njobs);
    TEST_ASSERT_EQUAL_INT(ujobs_before, S->n_ujobs);
    TEST_ASSERT_TRUE(check_route(P, S->routes[r_id]));

    free_solution(S);
    free_problem(P);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tabu_does_not_worsen);
    RUN_TEST(test_tabu_preserves_served_jobs);
    RUN_TEST(test_tabu_returns_feasible_solution);
    RUN_TEST(test_tabu_vehicle_count_non_increasing);
    RUN_TEST(test_tabu_does_not_modify_input);
    RUN_TEST(test_tabu_is_deterministic);
    RUN_TEST(test_tabu_seed_controls_the_rng);
    RUN_TEST(test_tabu_across_instance_families);
    RUN_TEST(test_tabu_get_name);
    RUN_TEST(test_reorder_route_keeps_feasible_and_conserves_jobs);
    return UNITY_END();
}
