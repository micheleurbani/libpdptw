#include <math.h>
#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/solution.h"
#include "../include/objectives.h"
#include "../include/config.h"

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

// A route's path, caches and job list, saved to compare against later.
typedef struct RouteSnapshot {
    int n, njobs;
    int path[MAX_NODES * 2], Q[MAX_NODES * 2];
    double D[MAX_NODES * 2], T[MAX_NODES * 2], b[MAX_NODES * 2];
    double wait[MAX_NODES * 2], slack[MAX_NODES * 2];
    int *jobs[MAX_NODES];
} RouteSnapshot;

static void snapshot_route(const Route* r, RouteSnapshot* s) {
    s->n = r->n;
    s->njobs = r->njobs;
    for (int i = 0; i < r->n; i++) {
        s->path[i] = r->path[i];
        s->Q[i] = r->Q[i];
        s->D[i] = r->D[i];
        s->T[i] = r->T[i];
        s->b[i] = r->b[i];
        s->wait[i] = r->wait[i];
        s->slack[i] = r->slack[i];
    }
    for (int i = 0; i < r->njobs; i++) s->jobs[i] = r->jobs[i];
}

static void assert_route_matches(const RouteSnapshot* s, const Route* r) {
    TEST_ASSERT_EQUAL_INT(s->n, r->n);
    TEST_ASSERT_EQUAL_INT(s->njobs, r->njobs);
    TEST_ASSERT_EQUAL_INT_ARRAY(s->path, r->path, s->n);
    TEST_ASSERT_EQUAL_INT_ARRAY(s->Q, r->Q, s->n);
    TEST_ASSERT_EQUAL_DOUBLE_ARRAY(s->D, r->D, s->n);
    TEST_ASSERT_EQUAL_DOUBLE_ARRAY(s->T, r->T, s->n);
    TEST_ASSERT_EQUAL_DOUBLE_ARRAY(s->b, r->b, s->n);
    TEST_ASSERT_EQUAL_DOUBLE_ARRAY(s->wait, r->wait, s->n);
    TEST_ASSERT_EQUAL_DOUBLE_ARRAY(s->slack, r->slack, s->n);
    for (int i = 0; i < s->njobs; i++) TEST_ASSERT_TRUE(s->jobs[i] == r->jobs[i]);
}

// Moving every job of one copied route to the front of another, and filling an
// empty copied route, grows both well past the length they had when copied.
// The original keeps its paths, caches and objectives, and the grown routes
// keep the nodes they had.
void test_copy_is_independent_when_its_routes_grow(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    Solution* S = create_solution(P);
    insertion_heuristic(S, 1.0, 0.5);
    static RouteSnapshot before[MAX_NODES];
    TEST_ASSERT_TRUE(P->M <= MAX_NODES && P->N + 2 <= 2 * MAX_NODES);
    for (int i = 0; i < P->M; i++) snapshot_route(S->routes[i], &before[i]);
    ObjectiveVector f_before = evaluate_objectives(S);

    Solution* C = copy_solution(S);
    // The first two routes that serve a job, and the first empty one.
    int from = -1, to = -1, empty = -1;
    for (int i = 0; i < P->M; i++) {
        if (C->routes[i]->njobs > 0) {
            if (from < 0) from = i; else if (to < 0) to = i;
        } else if (empty < 0) {
            empty = i;
        }
    }
    TEST_ASSERT_TRUE(from >= 0 && to >= 0 && empty >= 0);
    Route* rf = C->routes[from];
    Route* rt = C->routes[to];
    Route* re = C->routes[empty];

    // Move all of rf's jobs to the front of rt, in job-list order, and put a
    // second copy of each pair in the empty route: feasibility is beside the
    // point, only the storage is under test.
    const int moved = rf->njobs;
    int* jobs[MAX_NODES];
    for (int k = 0; k < moved; k++) jobs[k] = rf->jobs[k];
    for (int k = 0; k < moved; k++) {
        TEST_ASSERT_TRUE(remove_job(rf, jobs[k]));
        TEST_ASSERT_TRUE(insert_job(rt, jobs[k], 1, 2));
        TEST_ASSERT_TRUE(insert_job(re, jobs[k], 1, 2));
    }
    update_route(P, rf);
    update_route(P, rt);
    update_route(P, re);

    // rt is the moved pairs, last moved first, then its own path.
    TEST_ASSERT_EQUAL_INT(before[to].n + 2 * moved, rt->n);
    TEST_ASSERT_EQUAL_INT(before[to].njobs + moved, rt->njobs);
    for (int k = 0; k < moved; k++) {
        TEST_ASSERT_EQUAL_INT(jobs[moved - 1 - k][0], rt->path[1 + 2 * k]);
        TEST_ASSERT_EQUAL_INT(jobs[moved - 1 - k][1], rt->path[2 + 2 * k]);
        TEST_ASSERT_EQUAL_INT(jobs[moved - 1 - k][0], re->path[1 + 2 * k]);
    }
    TEST_ASSERT_EQUAL_INT_ARRAY(&before[to].path[1], &rt->path[1 + 2 * moved],
                                before[to].n - 1);
    TEST_ASSERT_EQUAL_INT(2, rf->n);
    TEST_ASSERT_EQUAL_INT(2 + 2 * moved, re->n);
    // The grown route's distance cache is the distance along its path.
    double distance = 0;
    for (int i = 1; i < rt->n; i++) distance += P->D[rt->path[i - 1]][rt->path[i]];
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, distance, rt->D[rt->n - 1]);

    for (int i = 0; i < P->M; i++) assert_route_matches(&before[i], S->routes[i]);
    ObjectiveVector f_after = evaluate_objectives(S);
    TEST_ASSERT_EQUAL_MEMORY(&f_before, &f_after, sizeof f_before);

    // Freeing the copy leaves the original whole.
    free_solution(C);
    for (int i = 0; i < P->M; i++) assert_route_matches(&before[i], S->routes[i]);
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

// A hand-built instance whose nodes lie on a line at the given x, so the
// distance and travel time between two nodes is |x_a - x_b|. Time windows are
// wide open and service is instant, so only capacity can make a position
// infeasible. Node 0 is the depot; node 2k+1 picks up job k and node 2k+2
// delivers it, with demand q[k].
static PDPTW* line_problem(int n_jobs, const double* x, const int* q, int Q) {
    int N = 2 * n_jobs + 1;
    PDPTW* P = create_problem(N, "line");
    P->M = n_jobs;
    P->Q = Q;
    for (int i = 0; i < N; i++) {
        P->coords[i][0] = x[i];
        P->coords[i][1] = 0;
        P->e[i] = 0;
        P->l[i] = 1000;
        P->s[i] = 0;
        P->q[i] = 0;
        for (int j = 0; j < N; j++) P->D[i][j] = fabs(x[i] - x[j]);
    }
    for (int k = 0; k < n_jobs; k++) {
        P->jobs[k][0] = 2 * k + 1;
        P->jobs[k][1] = 2 * k + 2;
        P->q[2 * k + 1] = q[k];
        P->q[2 * k + 2] = -q[k];
    }
    return P;
}

// Builds the route 0 -> nodes... -> 0 and refreshes its caches.
static Route* route_through(PDPTW* P, Solution* S, const int* nodes, int n) {
    Route* r = S->routes[0];
    r->path[0] = 0;
    for (int k = 0; k < n; k++) r->path[k + 1] = nodes[k];
    r->path[n + 1] = 0;
    r->n = n + 2;
    update_route(P, r);
    return r;
}

// Capacity 10. The route carries job A (load 8) from x=10 to x=20. Job B
// (load 5) is cheapest picked up at x=9 just before A and delivered at x=21
// just after it, but the vehicle would then carry 13 between A's nodes. That
// position must not be offered; any position that is offered fits.
void test_insertion_cost_rejects_a_job_spanning_a_loaded_node(void) {
    const double x[] = {0, 10, 20, 9, 21};
    const int q[] = {8, 5};
    PDPTW* P = line_problem(2, x, q, 10);
    Solution* S = create_solution(P);
    const int A[] = {1, 2};
    Route* r = route_through(P, S, A, 2);
    MoveCost cost = insertion_cost(P, r, P->jobs[1], 1, 1);
    TEST_ASSERT_LESS_THAN_DOUBLE(BIG_M, cost.cost);
    TEST_ASSERT_TRUE(insert_job(r, P->jobs[1], cost.pos[0], cost.pos[1]));
    update_route(P, r);
    TEST_ASSERT_TRUE(check_route(P, r));
    free_solution(S);
    free_problem(P);
}

// Capacity 10. The route serves job C (load 1) from x=5 to x=6, then job A
// (load 8) from x=10 to x=20. Job B (load 5) at x=5.5 and x=7 fits with no
// detour between C's pickup and delivery and between C's delivery and A's
// pickup: it rides with C (load 6) and is gone before A is loaded. That is the
// only position adding no distance, so it is the one offered.
void test_insertion_cost_accepts_a_delivery_just_before_a_loaded_node(void) {
    const double x[] = {0, 10, 20, 5, 6, 5.5, 7};
    const int q[] = {8, 1, 5};
    PDPTW* P = line_problem(3, x, q, 10);
    Solution* S = create_solution(P);
    const int CA[] = {3, 4, 1, 2};
    Route* r = route_through(P, S, CA, 4);
    MoveCost cost = insertion_cost(P, r, P->jobs[2], 1, 1);
    TEST_ASSERT_EQUAL_DOUBLE(0, cost.cost);
    TEST_ASSERT_EQUAL_INT(2, cost.pos[0]);
    TEST_ASSERT_EQUAL_INT(4, cost.pos[1]);
    free_solution(S);
    free_problem(P);
}

// With capacity 50 instead of 200, equal to the largest demand on lc101,
// capacity binds. Every route the Solomon insertion heuristic builds must still
// pass the feasibility check, so no position it took overloads the vehicle.
void test_insertion_heuristic_respects_a_binding_capacity(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    P->Q = 50;
    P->M = P->N / 2;
    Solution* S = create_solution(P);
    insertion_heuristic(S, mu, alpha);
    TEST_ASSERT_EQUAL_INT(0, S->n_ujobs);
    for (int i = 0; i < P->M; i++) {
        TEST_ASSERT_TRUE(check_route(P, S->routes[i]));
    }
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
    RUN_TEST(test_copy_is_independent_when_its_routes_grow);
    RUN_TEST(test_check_route_accepts_service_at_window_close);
    RUN_TEST(test_check_route_rejects_service_after_window_close);
    RUN_TEST(test_insertion_cost_rejects_a_job_spanning_a_loaded_node);
    RUN_TEST(test_insertion_cost_accepts_a_delivery_just_before_a_loaded_node);
    RUN_TEST(test_insertion_heuristic_respects_a_binding_capacity);
    return UNITY_END();
}