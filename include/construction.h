#ifndef CONSTRUCTION_H
#define CONSTRUCTION_H
#include "problem.h"
#include "solution.h"
#include "rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Solomon insertion parameters of random_construction's fallback. */
#define RANDOM_CONSTRUCTION_FALLBACK_MU 1.0
#define RANDOM_CONSTRUCTION_FALLBACK_ALPHA 1.0

/**
 * Build a solution by inserting jobs in a random order.
 *
 * Each job goes to the position with the smallest distance increase among the
 * routes opened so far. A new route is opened only when none of them can take
 * the job. If the fleet size would be exceeded, the construction starts over
 * with a new order, up to `max_retries` times; after that it falls back to the
 * Solomon insertion heuristic (insertion_heuristic with the
 * RANDOM_CONSTRUCTION_FALLBACK_* parameters), which may leave jobs unassigned.
 *
 * Every random choice is drawn from `rng`, so the same generator state gives
 * the same solution.
 *
 * @return A new solution with fresh route caches; free it with free_solution.
 */
Solution *random_construction(PDPTW *problem, Rng *rng, int max_retries);

#ifdef __cplusplus
}
#endif

#endif
