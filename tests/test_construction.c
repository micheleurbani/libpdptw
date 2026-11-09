#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/solution.h"
#include "../include/rng.h"
#include "../include/construction.h"

#define RETRIES 10

static PDPTW* P;

void setUp(void) {
    P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
}

void tearDown(void) {
    free_problem(P);
}

static Solution* construct(uint64_t seed) {
    Rng rng;
    rng_seed(&rng, seed);
    return random_construction(P, &rng, RETRIES);
}

// Every pickup and delivery node appears in exactly one route, once.
void test_every_job_is_served_exactly_once(void) {
    Solution* S = construct(1);
    int visits[P->N];
    for (int node = 0; node < P->N; node++) visits[node] = 0;
    for (int i = 0; i < P->M; i++) {
        Route* r = S->routes[i];
        for (int k = 1; k < r->n - 1; k++) visits[r->path[k]]++;
    }
    TEST_ASSERT_EQUAL_INT(0, S->n_ujobs);
    for (int node = 1; node < P->N; node++) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, visits[node], "node visits");
    }
    free_solution(S);
}

// Every route the construction builds passes the feasibility check.
void test_every_route_is_feasible(void) {
    Solution* S = construct(2);
    for (int i = 0; i < P->M; i++) {
        TEST_ASSERT_TRUE(check_route(P, S->routes[i]));
    }
    free_solution(S);
}

// With capacity 50 instead of 200, equal to the largest demand on lc101,
// capacity binds. Every route the construction builds must still pass the
// feasibility check, so no insertion it made overloads the vehicle.
void test_every_route_is_feasible_when_capacity_binds(void) {
    P->Q = 50;
    P->M = P->N / 2;
    for (uint64_t seed = 1; seed <= 20; seed++) {
        Solution* S = construct(seed);
        TEST_ASSERT_EQUAL_INT(0, S->n_ujobs);
        for (int i = 0; i < P->M; i++) {
            TEST_ASSERT_TRUE(check_route(P, S->routes[i]));
        }
        free_solution(S);
    }
}

static void assert_same_routes(Solution* a, Solution* b) {
    for (int i = 0; i < P->M; i++) {
        TEST_ASSERT_EQUAL_INT(a->routes[i]->n, b->routes[i]->n);
        TEST_ASSERT_EQUAL_INT_ARRAY(a->routes[i]->path, b->routes[i]->path,
                                    a->routes[i]->n);
    }
}

// The same seed gives the same solution; a different seed gives another one.
void test_same_seed_gives_same_solution(void) {
    Solution* a = construct(3);
    Solution* b = construct(3);
    assert_same_routes(a, b);
    Solution* c = construct(4);
    bool differs = false;
    for (int i = 0; i < P->M && !differs; i++) {
        Route* ra = a->routes[i];
        Route* rc = c->routes[i];
        if (ra->n != rc->n) { differs = true; break; }
        for (int k = 0; k < ra->n; k++) differs |= ra->path[k] != rc->path[k];
    }
    TEST_ASSERT_TRUE(differs);
    free_solution(a);
    free_solution(b);
    free_solution(c);
}

// On lc101 the random construction needs at least 13 vehicles. With a fleet
// of 12 every retry fails, so the fleet size is respected by falling back to
// the Solomon insertion heuristic, whose result is returned as is.
void test_falls_back_to_solomon_when_the_fleet_is_too_small(void) {
    P->M = 12;
    Solution* S = construct(5);
    Solution* solomon = create_solution(P);
    insertion_heuristic(solomon, RANDOM_CONSTRUCTION_FALLBACK_MU,
                        RANDOM_CONSTRUCTION_FALLBACK_ALPHA);
    assert_same_routes(S, solomon);
    TEST_ASSERT_EQUAL_INT(solomon->n_ujobs, S->n_ujobs);
    TEST_ASSERT_LESS_OR_EQUAL_INT(12, count_vehicles(S));
    free_solution(S);
    free_solution(solomon);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_every_job_is_served_exactly_once);
    RUN_TEST(test_every_route_is_feasible);
    RUN_TEST(test_every_route_is_feasible_when_capacity_binds);
    RUN_TEST(test_same_seed_gives_same_solution);
    RUN_TEST(test_falls_back_to_solomon_when_the_fleet_is_too_small);
    return UNITY_END();
}
