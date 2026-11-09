#include <math.h>
#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/solution.h"

static double mu;
static double alpha;

void setUp(void) {
    mu = 1;
    alpha = 1;
};

void tearDown(void) {

};

void test_empty_solution(void) {
    PDPTW* problem = read_pdptw_from_file("../data/pdp_100/lc102.txt");
    Solution* solution = create_solution(problem);
    // A fresh solution leaves every PD-pair unassigned, borrowing the pointers
    // from the problem rather than copying the pairs (see copy_solution).
    for (size_t i = 0; i < problem->N / 2; i++)
    {
        TEST_ASSERT_TRUE(problem->jobs[i] == solution->ujobs[i]);
    }
    // print_solution(solution);
    free_solution(solution);
    free_problem(problem);
}

void test_update_route(void) {
    PDPTW* problem = read_pdptw_from_file("../data/pdp_100/lc108.txt");
    Solution* S = create_solution(problem);
    // Add a job to route 0
    Route* r = S->routes[0];
    r->path[1] = S->ujobs[0][0];
    r->path[2] = S->ujobs[0][1];
    r->path[3] = 0;
    r->n = 4;
    update_route(problem, r);
    for (size_t i = 1; i < 4; i++)
    {
        TEST_ASSERT_EQUAL_FLOAT(
            problem->D[r->path[i - 1]][r->path[i]], r->D[i] - r->D[i - 1]
        );
    }
    // print_route(r, 1);
}

void test_insertion_cost(void) {
    PDPTW* problem = read_pdptw_from_file("../data/pdp_100/lc108.txt");
    Solution* S = create_solution(problem);
    Route* r = S->routes[0];
    bool update = update_route(problem, r);
    double alpha = 0.5;
    double mu = 1;
    MoveCost cost;
    cost = insertion_cost(problem, r, S->ujobs[0], mu, alpha);
    TEST_ASSERT_LESS_OR_EQUAL(BIG_M, cost.cost);
    TEST_ASSERT_TRUE(insert_job(r, S->ujobs[0], cost.pos[0], cost.pos[1]));
    update = update_route(problem, r);
    cost = insertion_cost(problem, r, S->ujobs[1], mu, alpha);
    TEST_ASSERT_LESS_OR_EQUAL(BIG_M, cost.cost);
    TEST_ASSERT_TRUE(insert_job(r, S->ujobs[1], cost.pos[0], cost.pos[1]));
    update = update_route(problem, r);
    cost = insertion_cost(problem, r, S->ujobs[2], mu, alpha);
    TEST_ASSERT_LESS_OR_EQUAL(BIG_M, cost.cost);
    TEST_ASSERT_TRUE(insert_job(r, S->ujobs[2], cost.pos[0], cost.pos[1]));
    update = update_route(problem, r);
    cost = insertion_cost(problem, r, S->ujobs[3], mu, alpha);
    TEST_ASSERT_LESS_OR_EQUAL(BIG_M, cost.cost);
    TEST_ASSERT_TRUE(insert_job(r, S->ujobs[3], cost.pos[0], cost.pos[1]));
    update = update_route(problem, r);
    cost = insertion_cost(problem, r, S->ujobs[4], mu, alpha);
    TEST_ASSERT_LESS_OR_EQUAL(BIG_M, cost.cost);
    TEST_ASSERT_TRUE(insert_job(r, S->ujobs[4], cost.pos[0], cost.pos[1]));
    update = update_route(problem, r);
    cost = insertion_cost(problem, r, S->ujobs[5], mu, alpha);
    TEST_ASSERT_LESS_OR_EQUAL(BIG_M, cost.cost);
    TEST_ASSERT_TRUE(insert_job(r, S->ujobs[5], cost.pos[0], cost.pos[1]));
    update = update_route(problem, r);
    cost = insertion_cost(problem, r, S->ujobs[6], mu, alpha);
    TEST_ASSERT_EQUAL(BIG_M, cost.cost);
    update = update_route(problem, r);
    // print_route(r, 1);
}

void test_insertion_step(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc109.txt");
    Solution* S = create_solution(P);
    bool done;
    int n_ujobs = S->n_ujobs;
    done = insertion_step(S, 0, mu, alpha);
    TEST_ASSERT_TRUE(done);
    TEST_ASSERT_GREATER_THAN(S->n_ujobs, n_ujobs);
    n_ujobs = S->n_ujobs;
    done = insertion_step(S, 0, mu, alpha);
    TEST_ASSERT_TRUE(done);
    TEST_ASSERT_GREATER_THAN(S->n_ujobs, n_ujobs);
    // print_solution(S);
}

void test_route_construction(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc108.txt");
    Solution* S = create_solution(P);
    int route_id = 0;
    TEST_ASSERT_TRUE(route_construction(S, route_id, mu, alpha));
    TEST_ASSERT_TRUE(check_route(P, S->routes[route_id]));
    // print_route(S->routes[route_id], 1);
}

void test_route_construction_long(void) {
    for (size_t i = 1; i < 9; i++)
    {
        // Compose file name
        char filename[50];
        sprintf(filename, "../data/pdp_100/lc10%ld.txt", i);
        PDPTW* P = read_pdptw_from_file(filename);
        Solution* S = create_solution(P);
        int route_id = 0;
        TEST_ASSERT_TRUE(route_construction(S, route_id, mu, alpha));
        TEST_ASSERT_TRUE(check_route(P, S->routes[route_id]));
        // print_route(S->routes[route_id], 1);
    }
}

void test_insertion_heuristic(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc102.txt");
    Solution* S = create_solution(P);
    insertion_heuristic(S, mu, alpha);
    // print_solution(S);
}

// The insertion heuristic has no lateness penalty, so insertion_cost must keep
// every constructed route strictly time-window feasible. This regression guard
// rebuilds the full multi-route solution on lc101 with the parameters that used
// to expose the push-forward bug (mu=1.0, alpha=0.5 produced 6 infeasible
// routes) and asserts every non-empty route passes check_route.
void test_insertion_heuristic_feasible(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    Solution* S = create_solution(P);
    insertion_heuristic(S, 1.0, 0.5);
    for (int i = 0; i < P->M; i++)
    {
        if (S->routes[i]->njobs)
        {
            TEST_ASSERT_TRUE(check_route(P, S->routes[i]));
        }
    }
}

void test_save_solution(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc103.txt");
    Solution* S = create_solution(P);
    insertion_heuristic(S, mu, alpha);
    TEST_ASSERT_TRUE(save_solution(S));
}

void test_worst_cost_removal(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc102.txt");
    Solution* S = create_solution(P);
    insertion_heuristic(S, mu, alpha);
    for (size_t i = 0; i < S->Nr; i++)
    {
        MoveCost cost = worst_cost_removal(S, S->routes[i], mu);
        TEST_ASSERT_GREATER_OR_EQUAL(-BIG_M, cost.cost);
        TEST_ASSERT_NOT_NULL(cost.job);
    }
}

void test_worst_cost_removal_long(void) {
    for (size_t i = 1; i < 9; i++)
    {
        // Compose file name
        char filename[50];
        sprintf(filename, "../data/pdp_100/lc10%ld.txt", i);
        PDPTW* P = read_pdptw_from_file(filename);
        Solution* S = create_solution(P);
        insertion_heuristic(S, mu, alpha);
        for (size_t i = 0; i < S->Nr; i++)
        {
            MoveCost cost = worst_cost_removal(S, S->routes[i], mu);
            TEST_ASSERT_GREATER_OR_EQUAL(-BIG_M, cost.cost);
            TEST_ASSERT_NOT_NULL(cost.job);
        }
    }
}

void test_pd_shift(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc108.txt");
    Solution* S = create_solution(P);
    insertion_heuristic(S, mu, alpha);
    int r1_id = 1;
    int r2_id = 2;
    // Store r1 and r2 paths for the test
    int r1[S->routes[r1_id]->n];
    for (size_t i = 0; i < S->routes[r1_id]->n; i++)
    {
        r1[i] = S->routes[r1_id]->path[i];
    }
    int r2[S->routes[r2_id]->n];
    for (size_t i = 0; i < S->routes[r2_id]->n; i++)
    {
        r2[i] = S->routes[r2_id]->path[i];
    }

    bool success = pd_shift(S, S->routes[r1_id], S->routes[r2_id], mu, alpha);
    if (success)
    {
        // Compare route 1 path before and after the shift
        int diff = 0;
        for (size_t i = 0; i < S->routes[r1_id]->n; i++)
        {
            if (S->routes[r1_id]->path[i] != r1[i]) diff++;
        }
        TEST_ASSERT_EQUAL_INT(2, diff); // Two nodes (pickup and delivery) must
                                        // have been removed from r1
        // Compare route 2 path before and after the shift
        diff = 0;
        for (size_t i = 0; i < S->routes[r2_id]->n; i++)
        {
            if (S->routes[r2_id]->path[i] != r2[i]) diff++;
        }
        TEST_ASSERT_EQUAL_INT(2, diff); // Two nodes (pickup and delivery) must
                                        // have been added to r2
    }
}

void test_pd_exchange(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc104.txt");
    Solution* S = create_solution(P);
    int r1_id = 0;
    int r2_id = 1;
    // Create route 1 manually
    for (size_t i = 0; i < 3; i++)
    {
        insert_job(S->routes[r1_id], P->jobs[i], i * 2 + 1, (i + 1) * 2);
    }
    update_route(P, S->routes[r1_id]);
    // Check solution feasibility
    TEST_ASSERT_TRUE(check_route(P, S->routes[r1_id]));
    // Create route 2 manually. i + 6 returns a feasible route
    for (size_t i = 0; i < 3; i++)
    {
        insert_job(S->routes[r2_id], P->jobs[i + 6], i * 2 + 1, (i + 1) * 2);
    }
    update_route(P, S->routes[r2_id]);
    // Check solution feasibility
    TEST_ASSERT_TRUE(check_route(P, S->routes[r2_id]));
    // Store r1 and r2 paths for the test
    int r1[S->routes[r1_id]->n];
    for (size_t i = 0; i < S->routes[r1_id]->n; i++)
    {
        r1[i] = S->routes[r1_id]->path[i];
    }
    int r2[S->routes[r2_id]->n];
    for (size_t i = 0; i < S->routes[r2_id]->n; i++)
    {
        r2[i] = S->routes[r2_id]->path[i];
    }
    // Try PD-exchange
    bool success = pd_exchange(S, S->routes[r1_id], S->routes[r2_id], mu, alpha);
    TEST_ASSERT_TRUE(success);
    if (success)
    {
        // Compare route 1 path before and after the shift
        int diff = 0;
        for (size_t i = 1; i < S->routes[r1_id]->n - 1; i++)
        {
            for (size_t j = 1; j < S->routes[r1_id]->n - 1; j++)
            {
                if (S->routes[r1_id]->path[i] == r1[j]) {
                    diff++;
                    break;
                }
            }
        }
        TEST_ASSERT_EQUAL_INT(2, S->routes[r1_id]->n - diff - 2); // Two nodes (pickup and delivery) must
                                        // have been removed from r1
        // Compare route 2 path before and after the shift
        diff = 0;
        for (size_t i = 1; i < S->routes[r2_id]->n - 1; i++)
        {
            for (size_t j = 1; j < S->routes[r2_id]->n - 1; j++)
            {
                if (S->routes[r2_id]->path[i] == r2[j]) {
                    diff++;
                    break;
                }
            }
        }
        TEST_ASSERT_EQUAL_INT(2, S->routes[r2_id]->n - diff - 2); // Two nodes (pickup and delivery) must
                                        // have been added to r2
    }
}

void test_pd_rearrange(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc104.txt");
    Solution* S = create_solution(P);
    // Add three jobs to route 0
    int r_id = 0;
    for (size_t i = 0; i < 3; i++)
    {
        // printf("%d %d", P->jobs[2][0], P->jobs[2][1]);
        insert_job(S->routes[r_id], P->jobs[i], i * 2 + 1, (i + 1) * 2);
    }
    // Check route feasibility
    TEST_ASSERT_TRUE(check_route(P, S->routes[0]));
    MoveCost c_removal = worst_cost_removal(S, S->routes[r_id], 1.0);
    // Arbitrarily remove one node from route r_id
    bool removed = remove_job(S->routes[r_id], c_removal.job);
    // Store r1 and r2 paths for the test
    int r1[S->routes[r_id]->n];
    for (size_t i = 0; i < S->routes[r_id]->n; i++)
    {
        r1[i] = S->routes[r_id]->path[i];
    }
    // Try PD-Rearrange
    bool success = pd_rearrange(S, S->routes[r_id], 1.0, 1.0);
    TEST_ASSERT_TRUE(success);
    if (success)
    {
        // Compare route 1 path before and after the shift
        int diff = 0;
        for (size_t i = 1; i < S->routes[r_id]->n - 1; i++)
        {
            if (S->routes[r_id]->path[i] != r1[i]) {
                diff++;
            }
        }
        TEST_ASSERT_GREATER_THAN(0, diff);
    }
}

void test_solution_cost(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc105.txt");
    Solution* S = create_solution(P);
    double alpha = 1;
    double beta = 1;
    double gamma = 1;
    double lambda = 1;
    double cost1 = get_solution_cost(S, alpha, beta, gamma, lambda);
    // The solution cost of an empty solution is always 0
    TEST_ASSERT_EQUAL(0, cost1);
    insertion_heuristic(S, mu, alpha);
    double cost2 = get_solution_cost(S, alpha, beta, gamma, lambda);
    // The solution cost of a non-empty solution is always greater than 0
    TEST_ASSERT_GREATER_THAN(0, cost2);
}

void test_copy_solution(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    Solution* S = create_solution(P);
    insertion_heuristic(S, 1.0, 0.5);
    double cost_src = get_solution_cost(S, 1.0, 1.0, 1.0, 1.0);

    Solution* C = copy_solution(S);
    // The clone must reproduce the same objective value.
    TEST_ASSERT_TRUE(fabs(cost_src - get_solution_cost(C, 1.0, 1.0, 1.0, 1.0)) < 1e-6);
    // The clone must own independent route and unassigned-job storage.
    TEST_ASSERT_TRUE(C->routes != S->routes);
    for (size_t i = 0; i < P->M; i++) {
        TEST_ASSERT_TRUE(C->routes[i] != S->routes[i]);
        TEST_ASSERT_EQUAL_INT(S->routes[i]->n, C->routes[i]->n);
        TEST_ASSERT_EQUAL_INT(S->routes[i]->njobs, C->routes[i]->njobs);
    }

    // Mutating the clone must not affect the source.
    int route_id = -1;
    for (size_t i = 0; i < P->M; i++) {
        if (C->routes[i]->njobs > 0) { route_id = i; break; }
    }
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, route_id);
    int src_n_before = S->routes[route_id]->n;
    pd_rearrange(C, C->routes[route_id], 1.0, 0.5);
    TEST_ASSERT_EQUAL_INT(src_n_before, S->routes[route_id]->n);
    TEST_ASSERT_TRUE(fabs(cost_src - get_solution_cost(S, 1.0, 1.0, 1.0, 1.0)) < 1e-6);

    free_solution(C);
    free_solution(S);
    free_problem(P);
}

// Builds the route depot -> pickup -> delivery -> depot for the first job of
// lc101 and refreshes its caches. Position 2 is the delivery.
static Route* single_job_route(PDPTW* P, Solution* S) {
    Route* r = S->routes[0];
    r->path[1] = S->ujobs[0][0];
    r->path[2] = S->ujobs[0][1];
    r->path[3] = 0;
    r->n = 4;
    update_route(P, r);
    return r;
}

// Service may begin exactly when a node's time window closes: the window is
// the closed interval [e, l].
void test_check_route_accepts_service_at_window_close(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    Solution* S = create_solution(P);
    Route* r = single_job_route(P, S);
    int delivery = r->path[2];
    P->l[delivery] = r->b[2];
    TEST_ASSERT_TRUE(check_route(P, r));
    free_solution(S);
    free_problem(P);
}

// Arriving after a node's window closes makes the route infeasible. The window
// opens at 0, so service begins on arrival, one time unit after the close.
void test_check_route_rejects_service_after_window_close(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    Solution* S = create_solution(P);
    Route* r = single_job_route(P, S);
    int delivery = r->path[2];
    P->e[delivery] = 0;
    P->l[delivery] = r->T[2] - 1;
    TEST_ASSERT_FALSE(check_route(P, r));
    free_solution(S);
    free_problem(P);
}

// write_solution writes each non-empty route, in order, as one "Route k :" line
// listing its nodes without the depot, to exactly the path it is given.
void test_write_solution_to_path(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    Solution* S = create_solution(P);
    insertion_heuristic(S, mu, alpha);
    const char* path = "write_solution_test.sol";
    remove(path);
    TEST_ASSERT_TRUE(write_solution(S, path));

    FILE* f = fopen(path, "r");
    TEST_ASSERT_NOT_NULL(f);
    char line[4096];
    int route = 0;  // index into S->routes
    int written = 0;
    while (fgets(line, sizeof(line), f)) {
        int k, offset;
        if (sscanf(line, "Route %d :%n", &k, &offset) != 1) continue;
        written++;
        TEST_ASSERT_EQUAL_INT(written, k);
        while (S->routes[route]->n <= 2) route++;
        Route* r = S->routes[route++];
        char* cursor = line + offset;
        for (int j = 1; j < r->n - 1; j++) {
            int node, used;
            TEST_ASSERT_EQUAL_INT(1, sscanf(cursor, "%d%n", &node, &used));
            TEST_ASSERT_EQUAL_INT(r->path[j], node);
            cursor += used;
        }
        int extra;
        TEST_ASSERT_EQUAL_INT(-1, sscanf(cursor, "%d", &extra));
    }
    fclose(f);
    TEST_ASSERT_EQUAL_INT(count_vehicles(S), written);
    remove(path);
    free_solution(S);
    free_problem(P);
}

// A path that cannot be opened is reported as a failure, not a crash.
void test_write_solution_reports_unwritable_path(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    Solution* S = create_solution(P);
    insertion_heuristic(S, mu, alpha);
    TEST_ASSERT_FALSE(write_solution(S, "no/such/directory/x.sol"));
    // Opens, but every write fails as on a full disk (Linux).
    TEST_ASSERT_FALSE(write_solution(S, "/dev/full"));
    free_solution(S);
    free_problem(P);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_empty_solution);
    RUN_TEST(test_update_route);
    RUN_TEST(test_insertion_cost);
    RUN_TEST(test_insertion_step);
    RUN_TEST(test_route_construction);
    RUN_TEST(test_route_construction_long);
    RUN_TEST(test_insertion_heuristic);
    RUN_TEST(test_insertion_heuristic_feasible);
    RUN_TEST(test_save_solution);
    RUN_TEST(test_write_solution_to_path);
    RUN_TEST(test_write_solution_reports_unwritable_path);
    RUN_TEST(test_worst_cost_removal);
    RUN_TEST(test_worst_cost_removal_long);
    RUN_TEST(test_pd_shift);
    RUN_TEST(test_pd_exchange);
    RUN_TEST(test_pd_rearrange);
    RUN_TEST(test_solution_cost);
    RUN_TEST(test_copy_solution);
    RUN_TEST(test_check_route_accepts_service_at_window_close);
    RUN_TEST(test_check_route_rejects_service_after_window_close);
    return UNITY_END();
}