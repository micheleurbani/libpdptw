#include "rng.h"

// PCG32 selects one of 2^63 streams with a second constant. One fixed stream
// keeps the seed the only input a caller has to record.
#define RNG_STREAM 54u

void rng_seed(Rng *rng, uint64_t seed) {
    pcg32_srandom_r(&rng->pcg, seed, RNG_STREAM);
}

uint32_t rng_next(Rng *rng) {
    return pcg32_random_r(&rng->pcg);
}

uint32_t rng_below(Rng *rng, uint32_t bound) {
    return pcg32_boundedrand_r(&rng->pcg, bound);
}
