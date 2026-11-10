#ifndef CROSSOVER_H
#define CROSSOVER_H
#include "solution.h"
#include "insertion.h"
#include "rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Crossover: build an offspring from two solutions `p` and `q` of the same
 * problem, drawing every random choice from `rng`.
 *
 * 1. Copy k ~ U[1, n - 1] of the n routes of `p` that serve a job, drawn
 *    uniformly (k = 1 when n = 1), in their order in `p`.
 * 2. Copy every route of `q` that shares no job with them, in route order,
 *    while the fleet has an unused route.
 * 3. Place every remaining job, in random order, at its best position for `c`
 *    (see Criterion in insertion.h) in a route that serves a job. A job that
 *    fits in none goes into an empty route, which then serves a job.
 *
 * @pre Both parents serve every job on feasible routes with fresh caches.
 * @return A new solution that serves every job on feasible routes with fresh
 *         caches; free it with free_solution. NULL if some job fits nowhere
 *         and no route is left to open.
 */
Solution *crossover(Solution *p, Solution *q, const Criterion *c, Rng *rng);

#ifdef __cplusplus
}
#endif

#endif
