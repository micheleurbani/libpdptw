#include "../lib/unity/unity.h"
#include "../include/problem.h"

void setUp(void) {

};

void tearDown(void) {

};

void test_read_problem_from_file(void) {
    PDPTW* P = read_pdptw_from_file("../data/pdp_100/lc101.txt");
    TEST_ASSERT_NOT_NULL(P);
    for (int i = 0; i < P->N; i++) {
        // Test time windows
        TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(P->e[i], P->l[i]);
        // Test distance
        for (int j = 0; j < P->N; j++) {
            TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.0, P->D[i][j]);
        }
    }
    // print_problem(P);
    free_problem(P);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_read_problem_from_file);
    return UNITY_END();
}