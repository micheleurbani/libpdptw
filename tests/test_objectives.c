#include <math.h>
#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/solution.h"
#include "../include/objectives.h"

void setUp(void) {}
void tearDown(void) {}

// Two jobs on a 3-4-5 grid, three vehicles, unit service at every node but
// the depot. Time windows are wide except where they force a wait.
static PDPTW* two_job_instance(void) {
    PDPTW* P = create_problem(5, "two-jobs");
    P->M = 3;
    P->Q = 10;
    const double xy[5][2] = {{0, 0}, {3, 0}, {3, 4}, {0, 6}, {0, 8}};
    const int q[5] = {0, 1, -1, 1, -1};
    const double e[5] = {0, 5, 0, 0, 12};
    for (int i = 0; i < 5; i++) {
        P->coords[i][0] = xy[i][0];
        P->coords[i][1] = xy[i][1];
        P->q[i] = q[i];
        P->e[i] = e[i];
        P->l[i] = 100;
        P->s[i] = i == 0 ? 0 : 1;
    }
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            P->D[i][j] = hypot(xy[i][0] - xy[j][0], xy[i][1] - xy[j][1]);
    P->jobs[0][0] = 1; P->jobs[0][1] = 2;
    P->jobs[1][0] = 3; P->jobs[1][1] = 4;
    return P;
}

static void serve(Solution* S, int route, int* job) {
    Route* r = S->routes[route];
    r->path[1] = job[0];
    r->path[2] = job[1];
    r->path[3] = 0;
    r->n = 4;
    r->jobs[0] = job;
    r->njobs = 1;
}

// Route A: 0 -> (3,0) -> (3,4) -> 0, distance 3+4+5 = 12. It arrives at the
// pickup at 3 and waits 2 for the window to open at 5; the delivery begins
// at 5+1+4 = 10 and the vehicle is back at 10+1+5 = 16.
// Route B: 0 -> (0,6) -> (0,8) -> 0, distance 6+2+8 = 16. The delivery is
// reached at 6+1+2 = 9 and waits 3 for its window at 12; back at 12+1+8 = 21.
// The third route stays empty and does not count as a vehicle.
void test_objective_vector_of_a_hand_built_solution(void) {
    PDPTW* P = two_job_instance();
    Solution* S = create_solution(P);
    serve(S, 0, P->jobs[0]);
    serve(S, 1, P->jobs[1]);
    S->n_ujobs = 0;

    ObjectiveVector f = evaluate_objectives(S);

    TEST_ASSERT_EQUAL_DOUBLE(2, f.f[OBJ_VEHICLES]);
    TEST_ASSERT_EQUAL_DOUBLE(28, f.f[OBJ_DISTANCE]);
    TEST_ASSERT_EQUAL_DOUBLE(21, f.f[OBJ_MAKESPAN]);
    TEST_ASSERT_EQUAL_DOUBLE(5, f.f[OBJ_WAITING]);
    free_solution(S);
    free_problem(P);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_objective_vector_of_a_hand_built_solution);
    return UNITY_END();
}
