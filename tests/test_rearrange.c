#include <math.h>
#include <stdlib.h>
#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/solution.h"
#include "../include/construction.h"
#include "../include/rearrange.h"

static PDPTW* P;
static Solution* S;

// A seeded construction of lc101: 14 to 18 routes, with empty routes left.
void setUp(void) {
    P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    Rng rng;
    rng_seed(&rng, 1);
    S = random_construction(P, &rng, 10);
}

void tearDown(void) {
    free_solution(S);
    free_problem(P);
}

static const enum Objective OBJECTIVES[] = {OBJ_DISTANCE, OBJ_MAKESPAN, OBJ_WAITING};

static double total_waiting(const Route* r) {
    double w = 0;
    for (int i = 0; i < r->n; i++) w += r->wait[i];
    return w;
}

// Feasibility recomputed from scratch, without check_route's messages.
static bool feasible(Route* r) {
    update_route(P, r);
    for (int i = 0; i < r->n; i++) {
        if (r->b[i] > P->l[r->path[i]] || r->Q[i] > P->Q) return false;
    }
    return true;
}

static Route* route_serving(Solution* s, int* job) {
    for (int i = 0; i < P->M; i++) {
        for (int j = 0; j < s->routes[i]->njobs; j++) {
            if (s->routes[i]->jobs[j] == job) return s->routes[i];
        }
    }
    return NULL;
}

// Every job is served exactly once, its pickup before its delivery, and every
// route is feasible.
static void assert_valid(Solution* s) {
    int* seen = calloc(P->N, sizeof(int));
    for (int i = 0; i < P->M; i++) {
        Route* r = s->routes[i];
        TEST_ASSERT_TRUE(feasible(r));
        for (int k = 1; k < r->n - 1; k++) seen[r->path[k]]++;
        for (int j = 0; j < r->njobs; j++) {
            int pick = -1, deliv = -1;
            for (int k = 0; k < r->n; k++) {
                if (r->path[k] == r->jobs[j][0]) pick = k;
                if (r->path[k] == r->jobs[j][1]) deliv = k;
            }
            TEST_ASSERT_TRUE(pick > 0 && deliv > pick);
        }
    }
    for (int k = 1; k < P->N; k++) TEST_ASSERT_EQUAL_INT_MESSAGE(1, seen[k], "node not served once");
    free(seen);
}

// What a position is judged on: the objective's value, then Δdistance.
typedef struct Score {
    double value;
    double delta_distance;
} Score;

static Score score_of(Route* after, double distance_before, double waiting_before,
                      enum Objective obj) {
    Score s = {0, after->D[after->n - 1] - distance_before};
    if (obj == OBJ_DISTANCE) s.value = s.delta_distance;
    if (obj == OBJ_MAKESPAN) s.value = after->T[after->n - 1];
    if (obj == OBJ_WAITING) s.value = total_waiting(after) - waiting_before;
    return s;
}

static bool better(Score a, Score b) {
    if (a.value != b.value) return a.value < b.value;
    return a.delta_distance < b.delta_distance;
}

// The best score over every feasible position in `r` without the job and in
// the first empty route, each recomputed from scratch.
static Score brute_force_best(Solution* s, Route* r, int* job, enum Objective obj) {
    Solution* c = copy_solution(s);
    Route* own = c->routes[r->id];
    remove_job(own, job);
    update_route(P, own);
    Route* candidates[2] = {own, NULL};
    for (int i = 0; i < P->M && own->n > 2; i++) {
        if (c->routes[i]->n == 2) {
            candidates[1] = c->routes[i];
            break;
        }
    }
    Score best = {INFINITY, INFINITY};
    for (int k = 0; k < 2 && candidates[k]; k++) {
        Route* t = candidates[k];
        for (int p = 1; p < t->n; p++) {
            for (int d = p + 1; d <= t->n; d++) {
                Solution* trial = copy_solution(c);
                Route* tt = trial->routes[t->id];
                insert_job(tt, job, p, d);
                if (feasible(tt)) {
                    Score sc = score_of(tt, t->D[t->n - 1], total_waiting(t), obj);
                    if (better(sc, best)) best = sc;
                }
                free_solution(trial);
            }
        }
    }
    free_solution(c);
    return best;
}

// Moving any job of the first routes lands it on a position as good as the
// best one a brute-force search finds, for every objective.
void test_job_lands_on_its_best_position(void) {
    int checked = 0;
    for (int o = 0; o < 3; o++) {
        for (int i = 0; i < 4; i++) {
            Route* r = S->routes[i];
            for (int j = 0; j < r->njobs; j++) {
                int* job = r->jobs[j];
                Score expected = brute_force_best(S, r, job, OBJECTIVES[o]);

                Solution* moved = copy_solution(S);
                Route* own = moved->routes[i];
                // The receiving route's distance and waiting before the move:
                // the own route without the job, or an empty route.
                Solution* removed = copy_solution(S);
                remove_job(removed->routes[i], job);
                update_route(P, removed->routes[i]);
                TEST_ASSERT_TRUE(move_to_best_position(moved, own, job, OBJECTIVES[o]));
                Route* to = route_serving(moved, job);
                Route* before = removed->routes[to->id];
                Score actual = score_of(to, before->D[before->n - 1], total_waiting(before),
                                        OBJECTIVES[o]);
                TEST_ASSERT_DOUBLE_WITHIN(1e-9, expected.value, actual.value);
                TEST_ASSERT_DOUBLE_WITHIN(1e-9, expected.delta_distance, actual.delta_distance);
                assert_valid(moved);
                free_solution(removed);
                free_solution(moved);
                checked++;
            }
        }
    }
    TEST_ASSERT_GREATER_THAN_INT(10, checked);
}

// The first route that serves exactly one job; if none does, the first empty
// route after its job is moved there from the first route with two or more.
static Route* single_job_route(Solution* s) {
    Route* single = NULL;
    for (int i = 0; i < P->M && !single; i++) {
        if (s->routes[i]->njobs == 1) single = s->routes[i];
    }
    if (single) return single;
    Route* from = NULL;
    for (int i = 0; i < P->M; i++) {
        if (!from && s->routes[i]->njobs > 1) from = s->routes[i];
        if (!single && s->routes[i]->n == 2) single = s->routes[i];
    }
    TEST_ASSERT_NOT_NULL(from);
    TEST_ASSERT_NOT_NULL(single);
    int* job = from->jobs[0];
    remove_job(from, job);
    update_route(P, from);
    insert_job(single, job, 1, 2);
    update_route(P, single);
    return single;
}

// A route's only job can only go back into that route, now empty: the move
// opens no other route.
void test_single_job_route_opens_no_other_route(void) {
    Route* single = single_job_route(S);
    const int vehicles = count_vehicles(S);
    int* job = single->jobs[0];
    TEST_ASSERT_TRUE(move_to_best_position(S, single, job, OBJ_MAKESPAN));
    TEST_ASSERT_EQUAL_PTR(single, route_serving(S, job));
    TEST_ASSERT_EQUAL_INT(vehicles, count_vehicles(S));
    assert_valid(S);
}

// Many seeded random rearranges keep every job served once on feasible
// routes, and never use more routes than the fleet size.
void test_random_rearrange_keeps_the_solution_valid(void) {
    Rng rng;
    rng_seed(&rng, 3);
    for (int step = 0; step < 300; step++) {
        Route* r;
        do {
            r = S->routes[rng_below(&rng, (uint32_t)P->M)];
        } while (r->njobs == 0);
        TEST_ASSERT_TRUE(random_rearrange(S, r, OBJECTIVES[step % 3], &rng));
    }
    assert_valid(S);
    TEST_ASSERT_TRUE(count_vehicles(S) <= P->M);
}

// A random route that serves at least `min_jobs` jobs, or NULL if none does.
static Route* random_route_with(Solution* s, int min_jobs, Rng* rng) {
    Route* eligible[P->M];
    int n = 0;
    for (int i = 0; i < P->M; i++) {
        if (s->routes[i]->njobs >= min_jobs) eligible[n++] = s->routes[i];
    }
    return n ? eligible[rng_below(rng, (uint32_t)n)] : NULL;
}

// Route rearrange moves k ~ U[2, jobs in the route] jobs: over many seeded
// moves k never leaves those bounds and reaches both, and every move keeps
// each job served once on feasible routes within the fleet size.
void test_route_rearrange_moves_between_two_and_all_jobs(void) {
    Rng rng;
    rng_seed(&rng, 5);
    bool saw_two = false, saw_all = false;
    for (int step = 0; step < 300; step++) {
        Route* r = random_route_with(S, 2, &rng);
        TEST_ASSERT_NOT_NULL(r);
        const int njobs = r->njobs;
        const int k = route_rearrange(S, r, OBJECTIVES[step % 3], &rng);
        TEST_ASSERT_TRUE(k >= 2 && k <= njobs);
        saw_two |= k == 2;
        saw_all |= k == njobs && njobs > 2;
        assert_valid(S);
        TEST_ASSERT_TRUE(count_vehicles(S) <= P->M);
    }
    TEST_ASSERT_TRUE(saw_two);
    TEST_ASSERT_TRUE(saw_all);
}

static void assert_same_routes(Solution* a, Solution* b) {
    for (int i = 0; i < P->M; i++) {
        TEST_ASSERT_EQUAL_INT(a->routes[i]->n, b->routes[i]->n);
        TEST_ASSERT_EQUAL_INT_ARRAY(a->routes[i]->path, b->routes[i]->path, a->routes[i]->n);
    }
}

// On a route with one job, route rearrange is random rearrange: from the same
// generator state it gives the same solution and leaves the same state.
void test_route_rearrange_of_a_single_job_route_is_random_rearrange(void) {
    const int id = single_job_route(S)->id;
    for (int o = 0; o < 3; o++) {
        Solution* a = copy_solution(S);
        Solution* b = copy_solution(S);
        Rng rng_a, rng_b;
        rng_seed(&rng_a, 11);
        rng_seed(&rng_b, 11);
        TEST_ASSERT_EQUAL_INT(1, route_rearrange(a, a->routes[id], OBJECTIVES[o], &rng_a));
        TEST_ASSERT_TRUE(random_rearrange(b, b->routes[id], OBJECTIVES[o], &rng_b));
        assert_same_routes(a, b);
        TEST_ASSERT_EQUAL_UINT32(rng_next(&rng_b), rng_next(&rng_a));
        free_solution(a);
        free_solution(b);
    }
}

// Split two jobs of a route of `s` off into an empty route, which then serves
// exactly those two, in a feasible order. Returns that route.
static Route* two_jobs_split_off(Solution* s) {
    Route* empty = NULL;
    for (int i = 0; i < P->M && !empty; i++) {
        if (s->routes[i]->n == 2) empty = s->routes[i];
    }
    TEST_ASSERT_NOT_NULL(empty);
    for (int i = 0; i < P->M; i++) {
        Route* from = s->routes[i];
        for (int a = 0; a < from->njobs; a++) {
            for (int b = a + 1; b < from->njobs; b++) {
                int* ja = from->jobs[a];
                int* jb = from->jobs[b];
                insert_job(empty, ja, 1, 2);
                insert_job(empty, jb, 3, 4);
                if (feasible(empty) && from->njobs > 2) {
                    remove_job(from, ja);
                    remove_job(from, jb);
                    update_route(P, from);
                    return empty;
                }
                remove_job(empty, ja);
                remove_job(empty, jb);
                update_route(P, empty);
            }
        }
    }
    TEST_FAIL_MESSAGE("no two jobs fit one route");
    return NULL;
}

// Two jobs split off a route cost less back in the other routes than on their
// own: route rearrange moves both (k ~ U[2, 2]) and empties their route.
void test_route_rearrange_can_empty_a_route(void) {
    Route* r = two_jobs_split_off(S);
    assert_valid(S);
    const int vehicles = count_vehicles(S);
    Rng rng;
    rng_seed(&rng, 13);
    TEST_ASSERT_EQUAL_INT(2, route_rearrange(S, r, OBJ_DISTANCE, &rng));
    TEST_ASSERT_EQUAL_INT(0, r->njobs);
    TEST_ASSERT_EQUAL_INT(vehicles - 1, count_vehicles(S));
    assert_valid(S);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_job_lands_on_its_best_position);
    RUN_TEST(test_single_job_route_opens_no_other_route);
    RUN_TEST(test_random_rearrange_keeps_the_solution_valid);
    RUN_TEST(test_route_rearrange_moves_between_two_and_all_jobs);
    RUN_TEST(test_route_rearrange_of_a_single_job_route_is_random_rearrange);
    RUN_TEST(test_route_rearrange_can_empty_a_route);
    return UNITY_END();
}
