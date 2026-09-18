#include "../lib/unity/unity.h"
#include "../include/problem.h"
#include "../include/route.h"

void setUp(void) {

};

void tearDown(void) {

};

void test_create_route(void) {
    Route* route = create_route(1, MAX_NODES);
    TEST_ASSERT_NOT_NULL(route);
    TEST_ASSERT_EQUAL_INT(1, route->id);
    TEST_ASSERT_EQUAL_INT(2, route->n);
    TEST_ASSERT_EQUAL_INT(0, route->path[0]);
    TEST_ASSERT_EQUAL_INT(0, route->path[1]);
    // print_route(route, 1);
    free_route(route);
}

void test_insert_node(void) {
    Route* route = create_route(0, MAX_NODES);
    // Try insertion in position 0, the depot position
    int success = insert_node(route, 1, 0);
    TEST_ASSERT_FALSE(success);
    // Try insertion in position n, the depot position
    success = insert_node(route, 1, route->n);
    TEST_ASSERT_FALSE(success);
    // Try multiple insertions
    success = insert_node(route, 1, 1);
    TEST_ASSERT_TRUE(success);
    success = insert_node(route, 2, 2);
    TEST_ASSERT_TRUE(success);
    success = insert_node(route, 3, 3);
    TEST_ASSERT_TRUE(success);
    free_route(route);
}

void test_remove_node(void) {
    Route* route = create_route(0, MAX_NODES);
    int success = remove_node(route, 0);
    TEST_ASSERT_FALSE(success);
    success = remove_node(route, route->n);
    TEST_ASSERT_FALSE(success);
    success = insert_node(route, 1, 1);
    success = insert_node(route, 2, 2);
    success = insert_node(route, 3, 3);
    TEST_ASSERT_TRUE(route->n == 5);
    success = remove_node(route, 3);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(route->n == 4);
    int actual1[] = {0, 1, 2, 0};
    for (size_t i = 0; i < route->n; i++)
    {
        TEST_ASSERT_EQUAL_INT(actual1[i], route->path[i]);
    }
    success = remove_node(route, 1);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(route->n == 3);
    int actual2[] = {0, 2, 0};
    for (size_t i = 0; i < route->n; i++)
    {
        TEST_ASSERT_EQUAL_INT(actual2[i], route->path[i]);
    }
    success = remove_node(route, 1);
    int actual3[] = {0, 0};
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(route->n == 2);
    for (size_t i = 0; i < route->n; i++)
    {
        TEST_ASSERT_EQUAL_INT(actual3[i], route->path[i]);
    }
    // Depot removal should fail
    success = remove_node(route, 1);
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_TRUE(route->n == 2);
}

void test_remove_node_by_id(void) {
    Route* route = create_route(0, MAX_NODES);
    int success = remove_node_by_id(route, 0);
    TEST_ASSERT_FALSE(success);
    success = insert_node(route, 1, 1);
    success = insert_node(route, 2, 2);
    success = insert_node(route, 3, 3);
    success = remove_node_by_id(route, 3);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(route->n == 4);
    // print_route(route, 0);
    int actual1[] = {0, 1, 2, 0};
    for (size_t i = 0; i < route->n; i++)
    {
        TEST_ASSERT_EQUAL_INT(actual1[i], route->path[i]);
    }
    success = remove_node_by_id(route, 1);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(route->n == 3);
    // print_route(route, 0);
    int actual2[] = {0, 2, 0};
    for (size_t i = 0; i < route->n; i++)
    {
        TEST_ASSERT_EQUAL_INT(actual2[i], route->path[i]);
    }
    success = remove_node_by_id(route, 2);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(route->n == 2);
    int actual3[] = {0, 0};
    for (size_t i = 0; i < route->n; i++)
    {
        TEST_ASSERT_EQUAL_INT(actual3[i], route->path[i]);
    }
}

void test_find_node_position(void) {
    Route* route = create_route(0, MAX_NODES);
    int success = remove_node_by_id(route, 0);
    TEST_ASSERT_FALSE(success);
    success = insert_node(route, 1, 1);
    success = insert_node(route, 2, 2);
    success = insert_node(route, 3, 3);
    int pos_3 = find_node_position(route, 3);
    TEST_ASSERT_EQUAL(3, pos_3);
    int pos_0 = find_node_position(route, 0);
    TEST_ASSERT_EQUAL(0, pos_0);
    int pos_5 = find_node_position(route, 5);
    TEST_ASSERT_EQUAL(-1, pos_5);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_create_route);
    RUN_TEST(test_insert_node);
    RUN_TEST(test_remove_node);
    RUN_TEST(test_remove_node_by_id);
    RUN_TEST(test_find_node_position);
    return UNITY_END();
}