#ifndef REARRANGE_H
#define REARRANGE_H
#include "solution.h"
#include "objectives.h"
#include "rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Move `job`, which `r` serves, to its best position for objective `obj`
 * (see better_position in insertion.h). The candidate routes are `r` itself,
 * without the job, and one empty route of the solution, if any; so the move
 * never exceeds the fleet size.
 *
 * The best position may be the one the job came from. If no position is
 * feasible, which cannot happen when `r` was feasible and distances obey the
 * triangle inequality, the job is put back where it was.
 *
 * @pre The routes' caches are fresh and `r` is feasible.
 * @post The caches of the routes involved are fresh.
 * @return Whether the job was placed at a best position.
 */
bool move_to_best_position(Solution *S, Route *r, int *job, enum Objective obj);

/**
 * Random rearrange: move one job of `r`, drawn uniformly from `rng`, to its
 * best position for `obj` with move_to_best_position.
 *
 * @pre `r` serves at least one job; see move_to_best_position.
 */
bool random_rearrange(Solution *S, Route *r, enum Objective obj, Rng *rng);

#ifdef __cplusplus
}
#endif

#endif
