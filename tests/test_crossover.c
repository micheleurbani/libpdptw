#include <stdlib.h>
#include <string.h>
#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/solution.h"
#include "../include/construction.h"
#include "../include/crossover.h"
#include "../include/insertion.h"
#include "../include/objectives.h"

#define NUM_PARENTS 6

static PDPTW* P;
static int fleet;  // P->M as read, which a test may change
static Solution* parents[NUM_PARENTS];

// Seeded constructions of lc101, each with 14 to 18 routes.
void setUp(void) {
    P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    fleet = P->M;
    Rng rng;
    rng_seed(&rng, 1);
    for (int i = 0; i < NUM_PARENTS; i++) parents[i] = random_construction(P, &rng, 10);
}

void tearDown(void) {
    P->M = fleet;
    for (int i = 0; i < NUM_PARENTS; i++) free_solution(parents[i]);
    free_problem(P);
}

// Two objective criteria, then weighted ones with weights per unit of lc101's
// typical objective values.
#define NUM_CRITERIA 4
static Criterion criterion_at(int i) {
    static const double WEIGHTS[2][NUM_OBJECTIVES] = {
        {0.25 / 8, 0.25 / 1900, 0.25 / 50, 0.25 / 800},
        {0.5 / 8, 0, 0.5 / 50, 0},
    };
    if (i == 0) return objective_criterion(OBJ_DISTANCE);
    if (i == 1) return objective_criterion(OBJ_WAITING);
    return weighted_criterion(WEIGHTS[i - 2]);
}

// Feasibility recomputed from scratch, without check_route's messages.
static bool feasible(Route* r) {
    for (int i = 0; i < r->n; i++) {
        if (r->b[i] > P->l[r->path[i]] || r->Q[i] > P->Q) return false;
    }
    return true;
}

// Every job is served exactly once, its pickup before its delivery, on
// feasible routes with fresh caches, within the fleet size.
static void assert_valid(Solution* s) {
    int* seen = calloc(P->N, sizeof(int));
    for (int i = 0; i < P->M; i++) {
        Route* r = s->routes[i];
        TEST_ASSERT_EQUAL_INT(i, r->id);
        double* cached = malloc(sizeof(double) * r->n);
        memcpy(cached, r->T, sizeof(double) * r->n);
        update_route(P, r);
        TEST_ASSERT_EQUAL_DOUBLE_ARRAY_MESSAGE(r->T, cached, r->n, "stale caches");
        free(cached);
        TEST_ASSERT_TRUE(feasible(r));
        TEST_ASSERT_EQUAL_INT((r->n - 2) / 2, r->njobs);
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
    TEST_ASSERT_EQUAL_INT(0, s->n_ujobs);
    free(seen);
}

// Many seeded crossovers between constructions, on every criterion, give
// offspring that serve every job once on feasible routes within the fleet.
void test_offspring_serve_every_job_once(void) {
    Rng rng;
    rng_seed(&rng, 3);
    int built = 0;
    for (int step = 0; step < 200; step++) {
        Solution* p = parents[rng_below(&rng, NUM_PARENTS)];
        Solution* q = parents[rng_below(&rng, NUM_PARENTS)];
        const Criterion c = criterion_at(step % NUM_CRITERIA);
        Solution* o = crossover(p, q, &c, &rng);
        if (!o) continue;
        assert_valid(o);
        built++;
        free_solution(o);
    }
    TEST_ASSERT_GREATER_THAN_INT(190, built);
}

static bool same_path(const Route* a, const Route* b) {
    return a->n == b->n && memcmp(a->path, b->path, sizeof(int) * a->n) == 0;
}

// Crossing a solution with itself copies k of its routes, then every other
// route, since none shares a job with them: the offspring has exactly the
// parent's routes.
void test_crossing_a_solution_with_itself_gives_back_its_routes(void) {
    Rng rng;
    rng_seed(&rng, 5);
    for (int i = 0; i < NUM_PARENTS; i++) {
        Solution* p = parents[i];
        const Criterion c = criterion_at(i % NUM_CRITERIA);
        Solution* o = crossover(p, p, &c, &rng);
        TEST_ASSERT_NOT_NULL(o);
        TEST_ASSERT_EQUAL_INT(count_vehicles(p), count_vehicles(o));
        for (int a = 0; a < P->M; a++) {
            if (p->routes[a]->njobs == 0) continue;
            int matches = 0;
            for (int b = 0; b < P->M; b++) matches += same_path(p->routes[a], o->routes[b]);
            TEST_ASSERT_EQUAL_INT(1, matches);
        }
        free_solution(o);
    }
}

// The same generator state gives the same offspring and leaves the same state.
void test_same_generator_state_gives_the_same_offspring(void) {
    for (int i = 0; i < NUM_CRITERIA; i++) {
        const Criterion c = criterion_at(i);
        Rng a, b;
        rng_seed(&a, 7 + i);
        rng_seed(&b, 7 + i);
        Solution* oa = crossover(parents[0], parents[1], &c, &a);
        Solution* ob = crossover(parents[0], parents[1], &c, &b);
        TEST_ASSERT_NOT_NULL(oa);
        TEST_ASSERT_NOT_NULL(ob);
        for (int r = 0; r < P->M; r++) TEST_ASSERT_TRUE(same_path(oa->routes[r], ob->routes[r]));
        TEST_ASSERT_EQUAL_UINT32(rng_next(&a), rng_next(&b));
        free_solution(oa);
        free_solution(ob);
    }
}

// lr101's constructions use its whole fleet of 25 vehicles, so the routes
// copied from both parents can take every vehicle, and a job left over may
// then fit nowhere: some crossovers return NULL, and every other offspring is
// valid.
void test_crossover_fails_when_no_route_is_left(void) {
    for (int i = 0; i < NUM_PARENTS; i++) free_solution(parents[i]);
    free_problem(P);
    P = read_pdptw_from_file("../data/pdp_100/lr101.txt");
    fleet = P->M;
    Rng rng;
    rng_seed(&rng, 1);
    for (int i = 0; i < NUM_PARENTS; i++) parents[i] = random_construction(P, &rng, 10);

    int failed = 0, built = 0;
    for (int step = 0; step < 300; step++) {
        Solution* p = parents[rng_below(&rng, NUM_PARENTS)];
        Solution* q = parents[rng_below(&rng, NUM_PARENTS)];
        const Criterion c = criterion_at(step % NUM_CRITERIA);
        Solution* o = crossover(p, q, &c, &rng);
        if (!o) {
            failed++;
            continue;
        }
        assert_valid(o);
        built++;
        free_solution(o);
    }
    TEST_ASSERT_GREATER_THAN_INT(0, failed);
    TEST_ASSERT_GREATER_THAN_INT(0, built);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_offspring_serve_every_job_once);
    RUN_TEST(test_crossing_a_solution_with_itself_gives_back_its_routes);
    RUN_TEST(test_same_generator_state_gives_the_same_offspring);
    RUN_TEST(test_crossover_fails_when_no_route_is_left);
    return UNITY_END();
}
