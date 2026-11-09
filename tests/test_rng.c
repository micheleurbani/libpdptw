#include "../lib/unity/unity.h"
#include <stdbool.h>
#include "../include/rng.h"

void setUp(void) {}
void tearDown(void) {}

// Two generators seeded alike produce the same draws, so a run is
// reproducible from its seed alone.
void test_same_seed_gives_same_sequence(void) {
    Rng a, b;
    rng_seed(&a, 42);
    rng_seed(&b, 42);
    for (int i = 0; i < 1000; i++) {
        TEST_ASSERT_EQUAL_UINT32(rng_next(&a), rng_next(&b));
    }
}

// rng_below(bound) draws from [0, bound) and reaches both ends of it.
void test_bounded_draws_fall_inside_their_bounds(void) {
    Rng rng;
    rng_seed(&rng, 7);
    const uint32_t bounds[] = {1, 2, 3, 10, 101};
    for (int k = 0; k < 5; k++) {
        bool seen_zero = false, seen_top = false;
        for (int i = 0; i < 10000; i++) {
            uint32_t x = rng_below(&rng, bounds[k]);
            TEST_ASSERT_LESS_THAN_UINT32(bounds[k], x);
            seen_zero |= x == 0;
            seen_top |= x == bounds[k] - 1;
        }
        TEST_ASSERT_TRUE(seen_zero);
        TEST_ASSERT_TRUE(seen_top);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_same_seed_gives_same_sequence);
    RUN_TEST(test_bounded_draws_fall_inside_their_bounds);
    return UNITY_END();
}
