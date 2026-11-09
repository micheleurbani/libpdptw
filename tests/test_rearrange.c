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

// A route's only job can only go back into that route, now empty: the move
// opens no other route.
void test_single_job_route_opens_no_other_route(void) {
    Route* single = NULL;
    for (int i = 0; i < P->M && !single; i++) {
        if (S->routes[i]->njobs > 1) continue;
        if (S->routes[i]->njobs == 1) single = S->routes[i];
    }
    if (!single) {
        // Make one: move the first job of route 0 into the first empty route.
        Route* from = S->routes[0];
        for (int i = 0; i < P->M && !single; i++) {
            if (S->routes[i]->n == 2) single = S->routes[i];
        }
        TEST_ASSERT_NOT_NULL(single);
        int* job = from->jobs[0];
        remove_job(from, job);
        update_route(P, from);
        insert_job(single, job, 1, 2);
        update_route(P, single);
    }
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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_job_lands_on_its_best_position);
    RUN_TEST(test_single_job_route_opens_no_other_route);
    RUN_TEST(test_random_rearrange_keeps_the_solution_valid);
    return UNITY_END();
}
