#include <math.h>
#include <stdlib.h>
#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/solution.h"
#include "../include/construction.h"
#include "../include/rearrange.h"
#include "../include/insertion.h"
#include "../include/objectives.h"

static PDPTW* P;
static Solution* S;
static int fleet;  // P->M as read, which a test may change

// A seeded construction of lc101: 14 to 18 routes, with empty routes left.
void setUp(void) {
    P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    fleet = P->M;
    Rng rng;
    rng_seed(&rng, 1);
    S = random_construction(P, &rng, 10);
}

void tearDown(void) {
    P->M = fleet;
    free_solution(S);
    free_problem(P);
}

static const enum Objective OBJECTIVES[] = {OBJ_DISTANCE, OBJ_MAKESPAN, OBJ_WAITING};

// Weighted criteria: each of two objectives alone and two mixes, with weights
// per unit of lc101's typical objective values, as MOMA builds them from
// normalised weight vectors.
#define NUM_WEIGHTS 4
static const double WEIGHTS[NUM_WEIGHTS][NUM_OBJECTIVES] = {
    {0.1, 0, 0, 0},
    {0, 0, 0.01, 0},
    {0.25 / 8, 0.25 / 1900, 0.25 / 50, 0.25 / 800},
    {0, 0.5 / 1900, 0, 0.5 / 800},
};

// Every criterion the operators are tested on: the three objectives the
// local searches move jobs for, then the weighted ones.
#define NUM_CRITERIA (3 + NUM_WEIGHTS)
static Criterion criterion_at(int i) {
    return i < 3 ? objective_criterion(OBJECTIVES[i]) : weighted_criterion(WEIGHTS[i - 3]);
}

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
                const Criterion c = objective_criterion(OBJECTIVES[o]);

                Solution* moved = copy_solution(S);
                Route* own = moved->routes[i];
                // The receiving route's distance and waiting before the move:
                // the own route without the job, or an empty route.
                Solution* removed = copy_solution(S);
                remove_job(removed->routes[i], job);
                update_route(P, removed->routes[i]);
                TEST_ASSERT_TRUE(move_to_best_position(moved, own, job, &c));
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

static double weighted_sum(Solution* s, const double* weights) {
    ObjectiveVector f = evaluate_objectives(s);
    double sum = 0;
    for (int k = 0; k < NUM_OBJECTIVES; k++) sum += weights[k] * f.f[k];
    return sum;
}

// Over every feasible position of `job` in `r` without it and in the first
// empty route, the candidates of move_to_best_position, each recomputed from
// scratch with evaluate_objectives: the smallest weighted sum of the whole
// solution, and the smallest Δdistance among the positions that reach it.
static Score brute_force_weighted_best(Solution* s, Route* r, int* job, const double* weights) {
    Solution* c = copy_solution(s);
    Route* own = c->routes[r->id];
    remove_job(own, job);
    update_route(P, own);
    const double distance_before = evaluate_objectives(c).f[OBJ_DISTANCE];
    Route* candidates[2] = {own, NULL};
    for (int i = 0; i < P->M && own->n > 2; i++) {
        if (c->routes[i]->n == 2) {
            candidates[1] = c->routes[i];
            break;
        }
    }
    Score found[2 * P->N * P->N];
    int n = 0;
    for (int k = 0; k < 2 && candidates[k]; k++) {
        Route* t = candidates[k];
        for (int p = 1; p < t->n; p++) {
            for (int d = p + 1; d <= t->n; d++) {
                Solution* trial = copy_solution(c);
                Route* tt = trial->routes[t->id];
                insert_job(tt, job, p, d);
                if (feasible(tt)) {
                    const double distance = evaluate_objectives(trial).f[OBJ_DISTANCE];
                    found[n++] = (Score){weighted_sum(trial, weights), distance - distance_before};
                }
                free_solution(trial);
            }
        }
    }
    Score best = {INFINITY, INFINITY};
    for (int i = 0; i < n; i++) best.value = fmin(best.value, found[i].value);
    for (int i = 0; i < n; i++) {
        if (found[i].value <= best.value + 1e-9) {
            best.delta_distance = fmin(best.delta_distance, found[i].delta_distance);
        }
    }
    free_solution(c);
    return best;
}

// Moving any job of the first routes with a weighted criterion lands it on a
// position where the weighted sum of the solution's objectives, recomputed
// from scratch, is as small as the best a brute-force search finds: the
// criterion's change in makespan and in vehicles is exact, not an estimate.
void test_weighted_job_lands_on_its_best_position(void) {
    int checked = 0;
    for (int w = 0; w < NUM_WEIGHTS; w++) {
        const Criterion c = weighted_criterion(WEIGHTS[w]);
        for (int i = 0; i < 4; i++) {
            Route* r = S->routes[i];
            for (int j = 0; j < r->njobs; j++) {
                int* job = r->jobs[j];
                Score expected = brute_force_weighted_best(S, r, job, WEIGHTS[w]);
                Solution* removed = copy_solution(S);
                remove_job(removed->routes[i], job);
                const double distance_before = evaluate_objectives(removed).f[OBJ_DISTANCE];
                Solution* moved = copy_solution(S);
                TEST_ASSERT_TRUE(move_to_best_position(moved, moved->routes[i], job, &c));
                TEST_ASSERT_DOUBLE_WITHIN(1e-9, expected.value, weighted_sum(moved, WEIGHTS[w]));
                TEST_ASSERT_DOUBLE_WITHIN(
                    1e-9, expected.delta_distance,
                    evaluate_objectives(moved).f[OBJ_DISTANCE] - distance_before);
                assert_valid(moved);
                free_solution(removed);
                free_solution(moved);
                checked++;
            }
        }
    }
    TEST_ASSERT_GREATER_THAN_INT(40, checked);
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
    const Criterion c = objective_criterion(OBJ_MAKESPAN);
    TEST_ASSERT_TRUE(move_to_best_position(S, single, job, &c));
    TEST_ASSERT_EQUAL_PTR(single, route_serving(S, job));
    TEST_ASSERT_EQUAL_INT(vehicles, count_vehicles(S));
    assert_valid(S);
}

// Many seeded random rearranges, on every criterion, keep every job served
// once on feasible routes, and never use more routes than the fleet size.
void test_random_rearrange_keeps_the_solution_valid(void) {
    Rng rng;
    rng_seed(&rng, 3);
    for (int step = 0; step < 300; step++) {
        Route* r;
        do {
            r = S->routes[rng_below(&rng, (uint32_t)P->M)];
        } while (r->njobs == 0);
        const Criterion c = criterion_at(step % NUM_CRITERIA);
        TEST_ASSERT_TRUE(random_rearrange(S, r, &c, &rng));
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
// moves, on every criterion, k never leaves those bounds and reaches both,
// and every move keeps each job served once on feasible routes within the
// fleet size.
void test_route_rearrange_moves_between_two_and_all_jobs(void) {
    Rng rng;
    rng_seed(&rng, 5);
    bool saw_two = false, saw_all = false;
    for (int step = 0; step < 300; step++) {
        Route* r = random_route_with(S, 2, &rng);
        TEST_ASSERT_NOT_NULL(r);
        const int njobs = r->njobs;
        const Criterion c = criterion_at(step % NUM_CRITERIA);
        const int k = route_rearrange(S, r, &c, &rng);
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
    for (int o = 0; o < NUM_CRITERIA; o++) {
        const Criterion c = criterion_at(o);
        Solution* a = copy_solution(S);
        Solution* b = copy_solution(S);
        Rng rng_a, rng_b;
        rng_seed(&rng_a, 11);
        rng_seed(&rng_b, 11);
        TEST_ASSERT_EQUAL_INT(1, route_rearrange(a, a->routes[id], &c, &rng_a));
        TEST_ASSERT_TRUE(random_rearrange(b, b->routes[id], &c, &rng_b));
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
    const Criterion c = objective_criterion(OBJ_DISTANCE);
    TEST_ASSERT_EQUAL_INT(2, route_rearrange(S, r, &c, &rng));
    TEST_ASSERT_EQUAL_INT(0, r->njobs);
    TEST_ASSERT_EQUAL_INT(vehicles - 1, count_vehicles(S));
    assert_valid(S);
}

// lc101 with a fleet of one vehicle per job, each job alone on its own route.
// Sets P->M to that fleet; set it back before freeing S. A test that fails
// first leaves tearDown to set it back.
static Solution* one_job_per_route(void) {
    P->M = P->N / 2;
    Solution* s = create_solution(P);
    for (int k = 0; k < P->M; k++) {
        insert_job(s->routes[k], P->jobs[k], 1, 2);
        update_route(P, s->routes[k]);
    }
    s->n_ujobs = 0;
    return s;
}

// Emptying one-job routes into the others lowers the number of vehicles, by
// as many routes as vehicle reduction reports, and keeps every job served
// once on feasible routes.
void test_vehicle_reduction_lowers_the_number_of_vehicles(void) {
    Solution* s = one_job_per_route();
    assert_valid(s);
    TEST_ASSERT_EQUAL_INT(P->M, count_vehicles(s));
    const int emptied = vehicle_reduction(s);
    TEST_ASSERT_GREATER_THAN_INT(0, emptied);
    TEST_ASSERT_EQUAL_INT(P->M - emptied, count_vehicles(s));
    assert_valid(s);
    free_solution(s);
    P->M = fleet;
}

static void assert_same_job_lists(Solution* a, Solution* b) {
    for (int i = 0; i < P->M; i++) {
        TEST_ASSERT_EQUAL_INT(a->routes[i]->njobs, b->routes[i]->njobs);
        for (int j = 0; j < a->routes[i]->njobs; j++) {
            TEST_ASSERT_EQUAL_PTR(a->routes[i]->jobs[j], b->routes[i]->jobs[j]);
        }
    }
}

// Vehicle reduction stops at its first failed attempt. A second call starts
// with that same attempt, so it empties no route and leaves the solution as
// it was: paths, job lists and caches alike.
void test_failed_vehicle_reduction_leaves_the_solution_unchanged(void) {
    vehicle_reduction(S);
    Solution* before = copy_solution(S);
    TEST_ASSERT_EQUAL_INT(0, vehicle_reduction(S));
    assert_same_routes(S, before);
    assert_same_job_lists(S, before);
    for (int i = 0; i < P->M; i++) {
        Route* r = S->routes[i];
        TEST_ASSERT_EQUAL_DOUBLE_ARRAY(before->routes[i]->T, r->T, r->n);
        TEST_ASSERT_EQUAL_DOUBLE_ARRAY(before->routes[i]->D, r->D, r->n);
    }
    free_solution(before);
}

// Whether `job`, served by `from`, has a feasible position in another route
// that serves a job.
static bool fits_elsewhere(Solution* s, Route* from, int* job) {
    for (int i = 0; i < P->M; i++) {
        Route* t = s->routes[i];
        if (t == from || t->njobs == 0) continue;
        InsertionPosition* out = malloc(sizeof(InsertionPosition) * max_insertion_positions(t));
        const int found = insertion_positions(P, t, job, out);
        free(out);
        if (found) return true;
    }
    return false;
}

// On a seeded construction of lc201, once vehicle reduction has stopped, the
// route with the fewest jobs has one job that fits in another route and one
// that fits nowhere. Listed first, the job that fits is moved before the
// other fails, and the failed attempt takes it back: the solution is as it
// was, job list order included.
void test_failed_vehicle_reduction_takes_back_the_jobs_it_moved(void) {
    free_solution(S);
    free_problem(P);
    P = read_pdptw_from_file("../data/pdp_100/lc201.txt");
    fleet = P->M;
    Rng rng;
    rng_seed(&rng, 2);
    S = random_construction(P, &rng, 10);
    vehicle_reduction(S);

    Route* fewest = NULL;
    for (int i = 0; i < P->M; i++) {
        Route* t = S->routes[i];
        if (t->njobs > 0 && (!fewest || t->njobs < fewest->njobs)) fewest = t;
    }
    TEST_ASSERT_EQUAL_INT(2, fewest->njobs);
    int** jobs = fewest->jobs;
    if (!fits_elsewhere(S, fewest, jobs[0])) {
        int* swap = jobs[0];
        jobs[0] = jobs[1];
        jobs[1] = swap;
    }
    TEST_ASSERT_TRUE(fits_elsewhere(S, fewest, jobs[0]));
    TEST_ASSERT_FALSE(fits_elsewhere(S, fewest, jobs[1]));

    Solution* before = copy_solution(S);
    TEST_ASSERT_EQUAL_INT(0, vehicle_reduction(S));
    assert_same_routes(S, before);
    assert_same_job_lists(S, before);
    assert_valid(S);
    free_solution(before);
}

// Vehicle reduction only moves jobs into routes that already serve one: a
// route empty before it is still empty after it.
void test_vehicle_reduction_never_opens_a_route(void) {
    bool was_empty[P->M];
    int empty = 0;
    for (int i = 0; i < P->M; i++) {
        was_empty[i] = S->routes[i]->njobs == 0;
        empty += was_empty[i];
    }
    TEST_ASSERT_GREATER_THAN_INT(0, empty);
    const int vehicles = count_vehicles(S);
    const int emptied = vehicle_reduction(S);
    for (int i = 0; i < P->M; i++) {
        if (was_empty[i]) TEST_ASSERT_EQUAL_INT(0, S->routes[i]->njobs);
    }
    TEST_ASSERT_EQUAL_INT(vehicles - emptied, count_vehicles(S));
    assert_valid(S);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_job_lands_on_its_best_position);
    RUN_TEST(test_weighted_job_lands_on_its_best_position);
    RUN_TEST(test_single_job_route_opens_no_other_route);
    RUN_TEST(test_random_rearrange_keeps_the_solution_valid);
    RUN_TEST(test_route_rearrange_moves_between_two_and_all_jobs);
    RUN_TEST(test_route_rearrange_of_a_single_job_route_is_random_rearrange);
    RUN_TEST(test_route_rearrange_can_empty_a_route);
    RUN_TEST(test_vehicle_reduction_lowers_the_number_of_vehicles);
    RUN_TEST(test_failed_vehicle_reduction_leaves_the_solution_unchanged);
    RUN_TEST(test_failed_vehicle_reduction_takes_back_the_jobs_it_moved);
    RUN_TEST(test_vehicle_reduction_never_opens_a_route);
    return UNITY_END();
}
