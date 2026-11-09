#include <math.h>
#include <stdlib.h>
#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/solution.h"
#include "../include/insertion.h"
#include "../include/construction.h"

void setUp(void) {}
void tearDown(void) {}

// Three jobs, capacity 1, unit service at every node but the depot.
// Route 0 -> 1 -> 2 -> 3 -> 4 -> 0 serves jobs (1,2) and (3,4); job (5,6) is
// the one to insert. Capacity 1 forbids carrying it past a loaded node, the
// window opening at 60 on node 3 makes the vehicle wait there, and node 4's
// window closing at 75 leaves little room to push it later.
static PDPTW* three_job_instance(void) {
    PDPTW* P = create_problem(7, "three-jobs");
    P->M = 3;
    P->Q = 1;
    const double xy[7][2] = {{0, 0}, {10, 0}, {20, 0}, {0, 10}, {0, 20}, {5, 5}, {15, 5}};
    const int q[7] = {0, 1, -1, 1, -1, 1, -1};
    const double e[7] = {0, 0, 0, 60, 0, 0, 0};
    const double l[7] = {200, 200, 200, 200, 75, 200, 200};
    for (int i = 0; i < 7; i++) {
        P->coords[i][0] = xy[i][0];
        P->coords[i][1] = xy[i][1];
        P->q[i] = q[i];
        P->e[i] = e[i];
        P->l[i] = l[i];
        P->s[i] = i == 0 ? 0 : 1;
    }
    for (int i = 0; i < 7; i++)
        for (int j = 0; j < 7; j++)
            P->D[i][j] = hypot(xy[i][0] - xy[j][0], xy[i][1] - xy[j][1]);  // P->T aliases D
    for (int k = 0; k < 3; k++) {
        P->jobs[k][0] = 2 * k + 1;
        P->jobs[k][1] = 2 * k + 2;
    }
    return P;
}

static Route* route_of(PDPTW* P, const int* path, int n) {
    Route* r = create_route(0, P->N + 2);
    for (int i = 0; i < n; i++) r->path[i] = path[i];
    r->n = n;
    update_route(P, r);
    return r;
}

static double total_waiting(const Route* r) {
    double w = 0;
    for (int i = 0; i < r->n; i++) w += r->wait[i];
    return w;
}

// Feasibility recomputed from scratch, without check_route's messages.
static bool feasible(PDPTW* P, Route* r) {
    update_route(P, r);
    for (int i = 0; i < r->n; i++) {
        if (r->b[i] > P->l[r->path[i]] || r->Q[i] > P->Q) return false;
    }
    return true;
}

typedef struct Coverage {
    int tried;            // positions tried
    int feasible;         // of which feasible
    int changes_waiting;  // of which change the route's waiting time
} Coverage;

// Insert `job` at every position in a copy of `r`, recompute the copy from
// scratch, and check that insertion_positions reports exactly the feasible
// positions, with their Δdistance, duration and Δwaiting. Adds to `cov` what
// the check covered.
static void check_against_recomputation(PDPTW* P, Route* r, int* job, Coverage* cov) {
    InsertionPosition* out = malloc(sizeof(InsertionPosition) * max_insertion_positions(r));
    int n_out = insertion_positions(P, r, job, out);
    int n_feasible = 0;
    for (int p = 1; p < r->n; p++) {
        for (int d = p + 1; d <= r->n; d++) {
            Route* c = route_of(P, r->path, r->n);
            insert_job(c, job, p, d);
            const bool ok = feasible(P, c);
            cov->tried++;
            const InsertionPosition* found = NULL;
            for (int k = 0; k < n_out; k++) {
                if (out[k].pickup == p && out[k].delivery == d) found = &out[k];
            }
            if (ok) {
                n_feasible++;
                const double delta_waiting = total_waiting(c) - total_waiting(r);
                cov->changes_waiting += fabs(delta_waiting) > 1e-6;
                TEST_ASSERT_NOT_NULL_MESSAGE(found, "a feasible position is missing");
                TEST_ASSERT_DOUBLE_WITHIN(1e-9, c->D[c->n - 1] - r->D[r->n - 1],
                                          found->delta_distance);
                TEST_ASSERT_DOUBLE_WITHIN(1e-9, c->T[c->n - 1], found->duration);
                TEST_ASSERT_DOUBLE_WITHIN(1e-9, delta_waiting, found->delta_waiting);
            } else {
                TEST_ASSERT_NULL_MESSAGE(found, "an infeasible position is reported");
            }
            free_route(c);
        }
    }
    TEST_ASSERT_EQUAL_INT(n_feasible, n_out);
    cov->feasible += n_feasible;
    free(out);
}

// The check covered infeasible positions and waiting-time changes.
static void assert_coverage(const Coverage* cov) {
    TEST_ASSERT_GREATER_THAN_INT(0, cov->changes_waiting);
    TEST_ASSERT_LESS_THAN_INT(cov->tried, cov->feasible);
}

// On a route with capacity, waiting and a tight window to respect, the
// evaluator reports exactly the feasible positions, with their exact effects.
void test_positions_match_a_recomputation(void) {
    PDPTW* P = three_job_instance();
    const int path[] = {0, 1, 2, 3, 4, 0};
    Route* r = route_of(P, path, 6);
    TEST_ASSERT_TRUE(feasible(P, r));
    Coverage cov = {0, 0, 0};
    check_against_recomputation(P, r, P->jobs[2], &cov);
    assert_coverage(&cov);
    free_route(r);
    free_problem(P);
}

// Worked example: an empty route has one position. 0 -> (5,5) -> (15,5) -> 0
// is 7.07 + 10 + 15.81 long and returns at 7.07 + 1 + 10 + 1 + 15.81.
void test_empty_route_has_one_position(void) {
    PDPTW* P = three_job_instance();
    const int path[] = {0, 0};
    Route* r = route_of(P, path, 2);
    InsertionPosition out[1];
    TEST_ASSERT_EQUAL_INT(1, max_insertion_positions(r));
    TEST_ASSERT_EQUAL_INT(1, insertion_positions(P, r, P->jobs[2], out));
    TEST_ASSERT_EQUAL_INT(1, out[0].pickup);
    TEST_ASSERT_EQUAL_INT(2, out[0].delivery);
    const double dist = sqrt(50) + 10 + sqrt(250);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, dist, out[0].delta_distance);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, dist + 2, out[0].duration);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0, out[0].delta_waiting);
    free_route(r);
    free_problem(P);
}

// On every route of a seeded construction of lc101, every job served
// elsewhere matches a recomputation from scratch.
void test_positions_match_a_recomputation_on_lc101(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    Rng rng;
    rng_seed(&rng, 1);
    Solution* S = random_construction(P, &rng, 10);
    Coverage cov = {0, 0, 0};
    for (int i = 0; i < P->M; i++) {
        Route* r = S->routes[i];
        if (r->n <= 2) continue;
        TEST_ASSERT_TRUE(feasible(P, r));
        for (int k = 0; k < P->N / 2; k++) {
            bool served = false;
            for (int j = 0; j < r->njobs; j++) served |= r->jobs[j] == P->jobs[k];
            if (!served) check_against_recomputation(P, r, P->jobs[k], &cov);
        }
    }
    assert_coverage(&cov);
    free_solution(S);
    free_problem(P);
}

// Ties on the objective go to the smaller Δdistance.
void test_better_position_breaks_ties_on_distance(void) {
    InsertionPosition a = {1, 2, 5, 100, 3};
    InsertionPosition b = {1, 2, 7, 90, 3};
    TEST_ASSERT_TRUE(better_position(&a, &b, OBJ_DISTANCE));
    TEST_ASSERT_TRUE(better_position(&b, &a, OBJ_MAKESPAN));
    TEST_ASSERT_TRUE(better_position(&a, &b, OBJ_WAITING));
    TEST_ASSERT_FALSE(better_position(&b, &a, OBJ_WAITING));
    TEST_ASSERT_FALSE(better_position(&a, &a, OBJ_WAITING));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_positions_match_a_recomputation);
    RUN_TEST(test_empty_route_has_one_position);
    RUN_TEST(test_positions_match_a_recomputation_on_lc101);
    RUN_TEST(test_better_position_breaks_ties_on_distance);
    return UNITY_END();
}
