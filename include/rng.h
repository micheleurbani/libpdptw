#ifndef RNG_H
#define RNG_H
#include <stdint.h>
#include "pcg_basic.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A seeded random number generator (PCG32). Its whole state lives in this
 * struct, which the caller owns and passes to whatever needs randomness:
 * libpdptw keeps no global generator and never calls rand().
 */
typedef struct Rng {
    pcg32_random_t pcg;
} Rng;

/** Seed `rng`. Generators seeded with the same value draw the same sequence. */
void rng_seed(Rng *rng, uint64_t seed);

/** Draw a uniformly distributed 32-bit value. */
uint32_t rng_next(Rng *rng);

/** Draw a uniformly distributed value in [0, bound), without modulo bias.
 * @pre bound > 0 */
uint32_t rng_below(Rng *rng, uint32_t bound);

#ifdef __cplusplus
}
#endif

#endif
