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

// Make S a construction of `instance` seeded with `seed`.
static void construct(const char* instance, uint64_t seed) {
    P = read_pdptw_from_file(instance);
    fleet = P->M;
    Rng rng;
    rng_seed(&rng, seed);
    S = random_construction(P, &rng, 10);
}

void tearDown(void) {
    P->M = fleet;
    free_solution(S);
    free_problem(P);
}

// A seeded construction of lc101: 14 to 18 routes, with empty routes left.
void setUp(void) {
    construct("../data/pdp_100/lc101.txt", 1);
}

// Replace setUp's S with a construction of `instance` seeded with `seed`.
static void reconstruct(const char* instance, uint64_t seed) {
    tearDown();
    construct(instance, seed);
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

// A copy of `s` without `job`, which route `id` serves, with fresh caches.
static Solution* without_job(Solution* s, int id, int* job) {
    Solution* c = copy_solution(s);
    remove_job(c->routes[id], job);
    update_route(P, c->routes[id]);
    return c;
}

// The first empty route of `s` after `own` loses a job, or -1 if `own` would be
// empty itself: the empty route move_to_best_position may put the job in.
static int empty_candidate(Solution* s, Route* own) {
    for (int i = 0; i < P->M && own->n > 2; i++) {
        if (s->routes[i]->n == 2) return i;
    }
    return -1;
}

// The best score over every feasible position of `job` in the `n` routes of
// `s` with ids `ids`, each recomputed from scratch. `s` doesn't serve `job`.
static Score best_score_in(Solution* s, const int* ids, int n, int* job, enum Objective obj) {
    Score best = {INFINITY, INFINITY};
    for (int k = 0; k < n; k++) {
        Route* t = s->routes[ids[k]];
        for (int p = 1; p < t->n; p++) {
            for (int d = p + 1; d <= t->n; d++) {
                Solution* trial = copy_solution(s);
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
    return best;
}

// The best score over every feasible position in `r` without the job and in
// the first empty route, each recomputed from scratch.
static Score brute_force_best(Solution* s, Route* r, int* job, enum Objective obj) {
    Solution* c = without_job(s, r->id, job);
    int ids[2] = {r->id, empty_candidate(c, c->routes[r->id])};
    Score best = best_score_in(c, ids, ids[1] < 0 ? 1 : 2, job, obj);
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

// Over every feasible position of `job` in the `n` routes of `s` with ids
// `ids`, each recomputed from scratch with evaluate_objectives: the smallest
// weighted sum of the whole solution, and the smallest Δdistance among the
// positions that reach it. `s` doesn't serve `job`.
static Score best_weighted_score_in(Solution* s, const int* ids, int n, int* job,
                                    const double* weights) {
    const double distance_before = evaluate_objectives(s).f[OBJ_DISTANCE];
    Score* found = malloc(sizeof(Score) * n * P->N * P->N);
    int count = 0;
    for (int k = 0; k < n; k++) {
        Route* t = s->routes[ids[k]];
        for (int p = 1; p < t->n; p++) {
            for (int d = p + 1; d <= t->n; d++) {
                Solution* trial = copy_solution(s);
                Route* tt = trial->routes[t->id];
                insert_job(tt, job, p, d);
                if (feasible(tt)) {
                    const double distance = evaluate_objectives(trial).f[OBJ_DISTANCE];
                    found[count++] =
                        (Score){weighted_sum(trial, weights), distance - distance_before};
                }
                free_solution(trial);
            }
        }
    }
    Score best = {INFINITY, INFINITY};
    for (int i = 0; i < count; i++) best.value = fmin(best.value, found[i].value);
    for (int i = 0; i < count; i++) {
        if (found[i].value <= best.value + 1e-9) {
            best.delta_distance = fmin(best.delta_distance, found[i].delta_distance);
        }
    }
    free(found);
    return best;
}

// Over every feasible position of `job` in `r` without it and in the first
// empty route, the candidates of move_to_best_position: see
// best_weighted_score_in.
static Score brute_force_weighted_best(Solution* s, Route* r, int* job, const double* weights) {
    Solution* c = without_job(s, r->id, job);
    int ids[2] = {r->id, empty_candidate(c, c->routes[r->id])};
    Score best = best_weighted_score_in(c, ids, ids[1] < 0 ? 1 : 2, job, weights);
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

// Whether `job` has a feasible position in `r`.
static bool fits_in(Route* r, int* job) {
    InsertionPosition* out = malloc(sizeof(InsertionPosition) * max_insertion_positions(r));
    const int found = insertion_positions(P, r, job, out);
    free(out);
    return found > 0;
}

// Whether `job`, served by `from`, has a feasible position in another route
// that serves a job.
static bool fits_elsewhere(Solution* s, Route* from, int* job) {
    for (int i = 0; i < P->M; i++) {
        Route* t = s->routes[i];
        if (t != from && t->njobs > 0 && fits_in(t, job)) return true;
    }
    return false;
}

// On a seeded construction of lc201, once vehicle reduction has stopped, the
// route with the fewest jobs has one job that fits in another route and one
// that fits nowhere. Listed first, the job that fits is moved before the
// other fails, and the failed attempt takes it back: the solution is as it
// was, job list order included.
void test_failed_vehicle_reduction_takes_back_the_jobs_it_moved(void) {
    reconstruct("../data/pdp_100/lc201.txt", 2);
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

// The ids of the first `most` routes of `s` that serve a job; returns how
// many there are.
static int serving_routes(Solution* s, int* ids, int most) {
    int n = 0;
    for (int i = 0; i < P->M && n < most; i++) {
        if (s->routes[i]->njobs > 0) ids[n++] = i;
    }
    return n;
}

// A job that `before` serves and `after` doesn't, or NULL if there is none.
static int* job_lost(const Route* before, const Route* after) {
    for (int j = 0; j < before->njobs; j++) {
        bool kept = false;
        for (int k = 0; k < after->njobs; k++) kept |= after->jobs[k] == before->jobs[j];
        if (!kept) return before->jobs[j];
    }
    return NULL;
}

// `after` is `before` with `job` inserted in route `to`: assert that the job
// sits at a best position there for criterion `criterion` (see criterion_at),
// over every feasible position in that route of `before`, recomputed from
// scratch. Both solutions' caches must be fresh.
static void assert_at_best_position(Solution* before, Solution* after, int to, int* job,
                                    int criterion) {
    if (criterion < 3) {
        const enum Objective obj = OBJECTIVES[criterion];
        Score expected = best_score_in(before, &to, 1, job, obj);
        Route* b = before->routes[to];
        Score actual = score_of(after->routes[to], b->D[b->n - 1], total_waiting(b), obj);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, expected.value, actual.value);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, expected.delta_distance, actual.delta_distance);
    } else {
        const double* weights = WEIGHTS[criterion - 3];
        Score expected = best_weighted_score_in(before, &to, 1, job, weights);
        const double distance_before = evaluate_objectives(before).f[OBJ_DISTANCE];
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, expected.value, weighted_sum(after, weights));
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, expected.delta_distance,
                                  evaluate_objectives(after).f[OBJ_DISTANCE] - distance_before);
    }
}

// A construction of lrc204, seeded with 2: five long routes with wide time
// windows, so that most jobs fit in another route, at many positions, which
// the criteria rank differently. On lc101 a job that fits in another route
// mostly fits at one position only.
static void construct_wide_windows(void) {
    reconstruct("../data/pdp_100/lrc204.txt", 2);
}

// PD-shift moves one job of r1 to r2, at its best position there for the
// criterion, whichever pair of routes and criterion it is applied to.
void test_pd_shift_moves_a_job_to_its_best_position_in_the_other_route(void) {
    construct_wide_windows();
    int ids[P->M];
    const int n = serving_routes(S, ids, P->M);
    Rng rng;
    rng_seed(&rng, 17);
    int checked = 0;
    for (int i = 0; i < NUM_CRITERIA; i++) {
        const Criterion c = criterion_at(i);
        for (int a = 0; a < n; a++) {
            for (int b = 0; b < n; b++) {
                if (a == b) continue;
                const int from = ids[a], to = ids[b];
                Solution* moved = copy_solution(S);
                if (pd_shift(moved, moved->routes[from], moved->routes[to], &c, &rng)) {
                    int* job = job_lost(S->routes[from], moved->routes[from]);
                    TEST_ASSERT_NOT_NULL(job);
                    TEST_ASSERT_EQUAL_INT(S->routes[from]->njobs - 1, moved->routes[from]->njobs);
                    TEST_ASSERT_EQUAL_PTR(moved->routes[to], route_serving(moved, job));
                    Solution* before = without_job(S, from, job);
                    assert_at_best_position(before, moved, to, job, i);
                    assert_valid(moved);
                    free_solution(before);
                    checked++;
                }
                free_solution(moved);
            }
        }
    }
    TEST_ASSERT_GREATER_THAN_INT(20, checked);
}

// Every route's caches are what update_route computes from its path, bit for
// bit. Call it before assert_valid, which refreshes them.
static void assert_fresh_caches(Solution* s) {
    for (int i = 0; i < P->M; i++) {
        Route* r = s->routes[i];
        Route* fresh = copy_route(r);
        update_route(P, fresh);
        const size_t n = (size_t)r->n;
        TEST_ASSERT_EQUAL_MEMORY(fresh->D, r->D, n * sizeof(double));
        TEST_ASSERT_EQUAL_MEMORY(fresh->T, r->T, n * sizeof(double));
        TEST_ASSERT_EQUAL_MEMORY(fresh->b, r->b, n * sizeof(double));
        TEST_ASSERT_EQUAL_MEMORY(fresh->wait, r->wait, n * sizeof(double));
        TEST_ASSERT_EQUAL_MEMORY(fresh->slack, r->slack, n * sizeof(double));
        TEST_ASSERT_EQUAL_MEMORY(fresh->Q, r->Q, n * sizeof(int));
        free_route(fresh);
    }
}

// `a` and `b` are the same route: path, job list and caches alike.
static void assert_same_route(const Route* a, const Route* b) {
    TEST_ASSERT_EQUAL_INT(a->n, b->n);
    TEST_ASSERT_EQUAL_INT_ARRAY(a->path, b->path, a->n);
    TEST_ASSERT_EQUAL_INT(a->njobs, b->njobs);
    for (int j = 0; j < a->njobs; j++) TEST_ASSERT_EQUAL_PTR(a->jobs[j], b->jobs[j]);
    const size_t n = (size_t)a->n;
    TEST_ASSERT_EQUAL_MEMORY(a->D, b->D, n * sizeof(double));
    TEST_ASSERT_EQUAL_MEMORY(a->T, b->T, n * sizeof(double));
    TEST_ASSERT_EQUAL_MEMORY(a->b, b->b, n * sizeof(double));
    TEST_ASSERT_EQUAL_MEMORY(a->wait, b->wait, n * sizeof(double));
    TEST_ASSERT_EQUAL_MEMORY(a->slack, b->slack, n * sizeof(double));
    TEST_ASSERT_EQUAL_MEMORY(a->Q, b->Q, n * sizeof(int));
}

// Every route of `after` is the route of `before` with the same id, except
// `skip1` and `skip2`, which may be NULL.
static void assert_same_routes_but(Solution* before, Solution* after, const Route* skip1,
                                   const Route* skip2) {
    for (int i = 0; i < P->M; i++) {
        if (after->routes[i] == skip1 || after->routes[i] == skip2) continue;
        assert_same_route(before->routes[i], after->routes[i]);
    }
}

// A random route of `s` other than `r`, which may be empty.
static Route* random_other_route(Solution* s, Route* r, Rng* rng) {
    Route* other;
    do {
        other = s->routes[rng_below(rng, (uint32_t)P->M)];
    } while (other == r);
    return other;
}

// Many seeded PD-shifts from a route that serves a job to any other route,
// empty ones included, on every criterion. Each leaves every job served once
// on feasible routes within the fleet size, with fresh caches. A shift that
// succeeds moves one job from r1 to r2 and touches no other route; one that
// fails leaves the solution exactly as it was. Both happen.
void test_pd_shift_keeps_the_solution_valid(void) {
    Rng rng;
    rng_seed(&rng, 7);
    int shifted = 0, failed = 0;
    for (int step = 0; step < 300; step++) {
        Route* r1 = random_route_with(S, 1, &rng);
        Route* r2 = random_other_route(S, r1, &rng);
        const Criterion c = criterion_at(step % NUM_CRITERIA);
        Solution* before = copy_solution(S);
        if (pd_shift(S, r1, r2, &c, &rng)) {
            shifted++;
            TEST_ASSERT_EQUAL_INT(before->routes[r1->id]->njobs - 1, r1->njobs);
            TEST_ASSERT_EQUAL_INT(before->routes[r2->id]->njobs + 1, r2->njobs);
            assert_same_routes_but(before, S, r1, r2);
        } else {
            failed++;
            assert_same_routes_but(before, S, NULL, NULL);
        }
        assert_fresh_caches(S);
        assert_valid(S);
        TEST_ASSERT_TRUE(count_vehicles(S) <= P->M);
        free_solution(before);
    }
    TEST_ASSERT_GREATER_THAN_INT(10, shifted);
    TEST_ASSERT_GREATER_THAN_INT(10, failed);
}

// Whether no job of `from` fits in `to`.
static bool none_fits(Route* from, Route* to) {
    for (int j = 0; j < from->njobs; j++) {
        if (fits_in(to, from->jobs[j])) return false;
    }
    return true;
}

// Two routes of lc101's construction that serve jobs, none of the first's
// fitting in the second: whichever job PD-shift draws, it fits nowhere. The
// shift moves nothing and leaves the solution exactly as it was.
void test_impossible_pd_shift_leaves_the_solution_unchanged(void) {
    Route *r1 = NULL, *r2 = NULL;
    for (int a = 0; a < P->M && !r2; a++) {
        for (int b = 0; b < P->M && !r2; b++) {
            Route *from = S->routes[a], *to = S->routes[b];
            if (a == b || from->njobs < 2 || to->njobs == 0) continue;
            if (none_fits(from, to)) {
                r1 = from;
                r2 = to;
            }
        }
    }
    TEST_ASSERT_NOT_NULL(r2);
    Solution* before = copy_solution(S);
    Rng rng;
    rng_seed(&rng, 19);
    for (int i = 0; i < NUM_CRITERIA; i++) {
        const Criterion c = criterion_at(i);
        TEST_ASSERT_FALSE(pd_shift(S, r1, r2, &c, &rng));
        assert_same_routes_but(before, S, NULL, NULL);
    }
    free_solution(before);
}

// The first route of `s` that serves no job, and the first that serves one.
static Route* first_empty_route(Solution* s) {
    for (int i = 0; i < P->M; i++) {
        if (s->routes[i]->njobs == 0) return s->routes[i];
    }
    TEST_FAIL_MESSAGE("no empty route");
    return NULL;
}

static Route* first_used_route(Solution* s) {
    for (int i = 0; i < P->M; i++) {
        if (s->routes[i]->njobs > 0) return s->routes[i];
    }
    TEST_FAIL_MESSAGE("no route serves a job");
    return NULL;
}

// PD-shift from a route that serves no job, or from a route to itself, moves
// nothing and draws no random number.
void test_pd_shift_from_an_empty_route_or_to_its_own_route_moves_nothing(void) {
    Route* empty = first_empty_route(S);
    Route* used = first_used_route(S);
    Solution* before = copy_solution(S);
    Rng rng, untouched;
    rng_seed(&rng, 23);
    rng_seed(&untouched, 23);
    for (int i = 0; i < NUM_CRITERIA; i++) {
        const Criterion c = criterion_at(i);
        TEST_ASSERT_FALSE(pd_shift(S, empty, used, &c, &rng));
        TEST_ASSERT_FALSE(pd_shift(S, used, used, &c, &rng));
    }
    assert_same_routes_but(before, S, NULL, NULL);
    TEST_ASSERT_EQUAL_UINT32(rng_next(&untouched), rng_next(&rng));
    free_solution(before);
}

// Apply PD-exchange to a copy of S, with r1 and r2 its routes `x` and `y`. If
// it swaps a job a of r1 with a job b of r2, assert where they went: a first,
// to its best position for criterion `criterion` (see criterion_at) in r2
// without b; then b to its best position in r1 without a, with a already in
// r2. Returns whether the jobs were swapped.
static bool exchange_lands_on_best_positions(int x, int y, int criterion, Rng* rng) {
    const Criterion c = criterion_at(criterion);
    Solution* moved = copy_solution(S);
    const bool swapped = pd_exchange(moved, moved->routes[x], moved->routes[y], &c, rng);
    if (swapped) {
        int* a = job_lost(S->routes[x], moved->routes[x]);
        int* b = job_lost(S->routes[y], moved->routes[y]);
        TEST_ASSERT_NOT_NULL(a);
        TEST_ASSERT_NOT_NULL(b);
        TEST_ASSERT_EQUAL_INT(S->routes[x]->njobs, moved->routes[x]->njobs);
        TEST_ASSERT_EQUAL_INT(S->routes[y]->njobs, moved->routes[y]->njobs);
        TEST_ASSERT_EQUAL_PTR(moved->routes[y], route_serving(moved, a));
        TEST_ASSERT_EQUAL_PTR(moved->routes[x], route_serving(moved, b));
        Solution* neither = without_job(S, x, a);
        remove_job(neither->routes[y], b);
        update_route(P, neither->routes[y]);
        Solution* a_placed = without_job(moved, x, b);
        assert_at_best_position(neither, a_placed, y, a, criterion);
        assert_at_best_position(a_placed, moved, x, b, criterion);
        assert_valid(moved);
        free_solution(a_placed);
        free_solution(neither);
    }
    free_solution(moved);
    return swapped;
}

// PD-exchange puts each of its two jobs at its best position in the other
// route, whichever pair of routes and criterion it is applied to.
void test_pd_exchange_puts_each_job_at_its_best_position_in_the_other_route(void) {
    construct_wide_windows();
    int ids[P->M];
    const int n = serving_routes(S, ids, P->M);
    Rng rng;
    rng_seed(&rng, 29);
    int checked = 0;
    for (int i = 0; i < NUM_CRITERIA; i++) {
        for (int k = 0; k < n; k++) {
            for (int l = 0; l < n; l++) {
                if (k != l) checked += exchange_lands_on_best_positions(ids[k], ids[l], i, &rng);
            }
        }
    }
    TEST_ASSERT_GREATER_THAN_INT(20, checked);
}

// The order of PD-exchange's two insertions shows only through a weighted
// criterion's change in makespan, and only for a few pairs of jobs: on
// lrc204, with makespan alone weighted, about 1 swap in 200. Over many draws
// on every pair of routes, the jobs land where r1's job going first puts
// them.
void test_pd_exchange_places_the_job_of_r1_first(void) {
    construct_wide_windows();
    const int makespan_alone = 3 + 1;  // WEIGHTS[1]
    int ids[P->M];
    const int n = serving_routes(S, ids, P->M);
    Rng rng;
    rng_seed(&rng, 31);
    for (int draw = 0; draw < 40; draw++) {
        for (int k = 0; k < n; k++) {
            for (int l = 0; l < n; l++) {
                if (k != l) exchange_lands_on_best_positions(ids[k], ids[l], makespan_alone, &rng);
            }
        }
    }
}

// Many seeded PD-exchanges between two routes that serve a job, on every
// criterion. Each leaves every job served once on feasible routes within the
// fleet size, with fresh caches. An exchange that succeeds keeps each route's
// number of jobs and touches no other route; one that fails leaves the
// solution exactly as it was. Both happen.
void test_pd_exchange_keeps_the_solution_valid(void) {
    Rng rng;
    rng_seed(&rng, 37);
    int swapped = 0, failed = 0;
    for (int step = 0; step < 300; step++) {
        Route* r1 = random_route_with(S, 1, &rng);
        Route* r2;
        do {
            r2 = random_route_with(S, 1, &rng);
        } while (r2 == r1);
        const Criterion c = criterion_at(step % NUM_CRITERIA);
        Solution* before = copy_solution(S);
        if (pd_exchange(S, r1, r2, &c, &rng)) {
            swapped++;
            TEST_ASSERT_EQUAL_INT(before->routes[r1->id]->njobs, r1->njobs);
            TEST_ASSERT_EQUAL_INT(before->routes[r2->id]->njobs, r2->njobs);
            assert_same_routes_but(before, S, r1, r2);
        } else {
            failed++;
            assert_same_routes_but(before, S, NULL, NULL);
        }
        assert_fresh_caches(S);
        assert_valid(S);
        TEST_ASSERT_TRUE(count_vehicles(S) <= P->M);
        free_solution(before);
    }
    TEST_ASSERT_GREATER_THAN_INT(10, swapped);
    TEST_ASSERT_GREATER_THAN_INT(10, failed);
}

// Whether `job` fits in `r` without `leaving`, which `r` serves.
static bool fits_in_without(Route* r, int* leaving, int* job) {
    Route* without = copy_route(r);
    remove_job(without, leaving);
    update_route(P, without);
    const bool fits = fits_in(without, job);
    free_route(without);
    return fits;
}

// Whether `job` fits in `r` without any one of its jobs.
static bool fits_without_any(Route* r, int* job) {
    for (int j = 0; j < r->njobs; j++) {
        if (fits_in_without(r, r->jobs[j], job)) return true;
    }
    return false;
}

// On lc101's construction, a job b that fits nowhere in a route r1, even
// without any one of r1's jobs, is put alone on a route r2. Whichever job a of
// r1 PD-exchange draws, a fits in r2 without b, which is empty, and b then
// fits nowhere in r1 without a: the exchange takes a back out of r2, moves
// nothing and leaves the solution exactly as it was.
void test_failed_pd_exchange_takes_back_the_job_it_placed(void) {
    Route *r1 = NULL, *home = NULL;
    int* b = NULL;
    for (int i = 0; i < P->M && !b; i++) {
        Route* t = S->routes[i];
        for (int k = 0; k < P->M && t->njobs >= 2 && !b; k++) {
            Route* u = S->routes[k];
            for (int j = 0; j < u->njobs && u != t && !b; j++) {
                if (!fits_without_any(t, u->jobs[j])) {
                    r1 = t;
                    home = u;
                    b = u->jobs[j];
                }
            }
        }
    }
    TEST_ASSERT_NOT_NULL(b);
    Route* r2 = home;
    if (home->njobs > 1) {
        r2 = first_empty_route(S);
        remove_job(home, b);
        update_route(P, home);
        insert_job(r2, b, 1, 2);
        update_route(P, r2);
    }
    assert_valid(S);
    Route* empty = first_empty_route(S);
    for (int j = 0; j < r1->njobs; j++) TEST_ASSERT_TRUE(fits_in(empty, r1->jobs[j]));

    Solution* before = copy_solution(S);
    Rng rng;
    rng_seed(&rng, 41);
    for (int i = 0; i < NUM_CRITERIA; i++) {
        const Criterion c = criterion_at(i);
        TEST_ASSERT_FALSE(pd_exchange(S, r1, r2, &c, &rng));
        assert_same_routes_but(before, S, NULL, NULL);
    }
    free_solution(before);
}

// PD-exchange with a route that serves no job, on either side, or between a
// route and itself, moves nothing and draws no random number.
void test_pd_exchange_with_an_empty_route_or_within_a_route_moves_nothing(void) {
    Route* empty = first_empty_route(S);
    Route* used = first_used_route(S);
    Solution* before = copy_solution(S);
    Rng rng, untouched;
    rng_seed(&rng, 43);
    rng_seed(&untouched, 43);
    for (int i = 0; i < NUM_CRITERIA; i++) {
        const Criterion c = criterion_at(i);
        TEST_ASSERT_FALSE(pd_exchange(S, empty, used, &c, &rng));
        TEST_ASSERT_FALSE(pd_exchange(S, used, empty, &c, &rng));
        TEST_ASSERT_FALSE(pd_exchange(S, used, used, &c, &rng));
    }
    assert_same_routes_but(before, S, NULL, NULL);
    TEST_ASSERT_EQUAL_UINT32(rng_next(&untouched), rng_next(&rng));
    free_solution(before);
}

// One seeded PD-shift (odd steps) or PD-exchange (even steps) on `s`, between
// random routes drawn from `rng`. Returns whether it moved anything.
static bool random_pd_move(Solution* s, int step, Rng* rng) {
    const Criterion c = criterion_at(step % NUM_CRITERIA);
    Route* r1 = random_route_with(s, 1, rng);
    if (step % 2) return pd_shift(s, r1, random_other_route(s, r1, rng), &c, rng);
    Route* r2;
    do {
        r2 = random_route_with(s, 1, rng);
    } while (r2 == r1);
    return pd_exchange(s, r1, r2, &c, rng);
}

// From the same generator state, a run of PD-shifts and PD-exchanges on two
// copies of a solution makes the same moves, and leaves the generators in the
// same state.
void test_pd_operators_give_the_same_result_from_the_same_generator_state(void) {
    Solution* a = copy_solution(S);
    Solution* b = copy_solution(S);
    Rng rng_a, rng_b;
    rng_seed(&rng_a, 47);
    rng_seed(&rng_b, 47);
    int moves = 0;
    for (int step = 0; step < 200; step++) {
        const bool moved = random_pd_move(a, step, &rng_a);
        TEST_ASSERT_EQUAL(moved, random_pd_move(b, step, &rng_b));
        assert_same_routes_but(a, b, NULL, NULL);
        moves += moved;
    }
    TEST_ASSERT_GREATER_THAN_INT(20, moves);
    TEST_ASSERT_EQUAL_UINT32(rng_next(&rng_a), rng_next(&rng_b));
    free_solution(a);
    free_solution(b);
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
    RUN_TEST(test_pd_shift_moves_a_job_to_its_best_position_in_the_other_route);
    RUN_TEST(test_pd_shift_keeps_the_solution_valid);
    RUN_TEST(test_impossible_pd_shift_leaves_the_solution_unchanged);
    RUN_TEST(test_pd_shift_from_an_empty_route_or_to_its_own_route_moves_nothing);
    RUN_TEST(test_pd_exchange_puts_each_job_at_its_best_position_in_the_other_route);
    RUN_TEST(test_pd_exchange_places_the_job_of_r1_first);
    RUN_TEST(test_pd_exchange_keeps_the_solution_valid);
    RUN_TEST(test_failed_pd_exchange_takes_back_the_job_it_placed);
    RUN_TEST(test_pd_exchange_with_an_empty_route_or_within_a_route_moves_nothing);
    RUN_TEST(test_pd_operators_give_the_same_result_from_the_same_generator_state);
    return UNITY_END();
}
