#ifndef REARRANGE_H
#define REARRANGE_H
#include "solution.h"
#include "objectives.h"
#include "insertion.h"
#include "rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Move `job`, which `r` serves, to its best position for criterion `c` (see
 * Criterion in insertion.h). The candidate routes are `r` itself,
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
bool move_to_best_position(Solution *S, Route *r, int *job, const Criterion *c);

/**
 * Random rearrange: move one job of `r`, drawn uniformly from `rng`, to its
 * best position for `c` with move_to_best_position.
 *
 * @pre `r` serves at least one job; see move_to_best_position.
 */
bool random_rearrange(Solution *S, Route *r, const Criterion *c, Rng *rng);

/**
 * Route rearrange: remove k jobs of `r`, drawn from `rng` with
 * k ~ U[2, r->njobs], then reinsert them one at a time, in random order, each
 * at its best position for `c` (see Criterion in insertion.h). The
 * candidate routes for each job are every route that serves a job, `r` itself
 * and one empty route, so `r` may end up empty and the move never exceeds the
 * fleet size; a move that fills one empty route may open another.
 *
 * A route with a single job is rearranged with random_rearrange instead.
 *
 * If some job has no feasible position, every job moved so far is taken back
 * and the solution is restored to its routes before the move, paths and the
 * job list of `r` alike.
 *
 * @pre The routes' caches are fresh, every route is feasible, and `r` serves
 *      at least one job.
 * @post The caches of the routes involved are fresh.
 * @return The number of jobs moved, or 0 if the move was taken back.
 */
int route_rearrange(Solution *S, Route *r, const Criterion *c, Rng *rng);

/**
 * PD-shift (Li & Lim 2003): move one job of `r1`, drawn uniformly from `rng`,
 * to its best position for `c` in `r2` (see Criterion in insertion.h). `r2`
 * may be empty, in which case the move opens it, and moving `r1`'s only job
 * empties `r1`; no other route is touched, so the move never exceeds the
 * fleet size.
 *
 * All or nothing: if the job has no feasible position in `r2`, it is put
 * back where it was, in `r1`'s path and job list alike, and nothing moves.
 *
 * If `r1` serves no job, or `r1` and `r2` are the same route, nothing moves
 * and no random number is drawn: moving a job within its own route is what
 * random_rearrange does.
 *
 * @pre The routes' caches are fresh and every route is feasible.
 * @post The caches of `r1` and `r2` are fresh.
 * @return Whether the job moved.
 */
bool pd_shift(Solution *S, Route *r1, Route *r2, const Criterion *c, Rng *rng);

/**
 * PD-exchange (Li & Lim 2003): swap a job a of `r1` with a job b of `r2`,
 * each drawn uniformly from `rng`, a first. Both leave their routes, then
 * each goes to its best position for `c` (see Criterion in insertion.h) in
 * the other route, in this order: a into `r2` without b, then b into `r1`
 * without a, with a already in `r2`. The order matters only to a weighted
 * criterion, whose change in makespan is measured against every route. No
 * other route is touched.
 *
 * All or nothing: if a or b has no feasible position, both routes are
 * restored as they were, paths and job lists alike, and nothing moves.
 *
 * If either route serves no job, or `r1` and `r2` are the same route, nothing
 * moves and no random number is drawn.
 *
 * @pre The routes' caches are fresh and every route is feasible.
 * @post The caches of `r1` and `r2` are fresh.
 * @return Whether the jobs were swapped.
 */
bool pd_exchange(Solution *S, Route *r1, Route *r2, const Criterion *c, Rng *rng);

/**
 * Vehicle reduction: empty the route with the fewest jobs (the first such
 * route on ties) by moving each of its jobs, in the order of its job list, to
 * its best position for OBJ_VEHICLES in another route that serves a job: the
 * position that adds the least route duration, with ties going to the
 * smaller Δdistance (see better_position in insertion.h). It never opens a
 * route.
 *
 * All or nothing: if some job has no feasible position, every job moved so
 * far is taken back, the route is restored as it was, paths and job list
 * alike, and vehicle reduction stops. While it succeeds, it repeats on the
 * next route with the fewest jobs. It draws no random numbers.
 *
 * @pre The routes' caches are fresh and every route is feasible.
 * @post The caches of the routes involved are fresh.
 * @return The number of routes emptied.
 */
int vehicle_reduction(Solution *S);

#ifdef __cplusplus
}
#endif

#endif
