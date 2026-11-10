#ifndef PLACEMENT_H
#define PLACEMENT_H
/* Internal to libpdptw: placing one job at its best position, shared by the
 * rearrange operators and crossover. */
#include "insertion.h"

/*
 * Insert `job` at its best position for `c` over the `n` candidate routes of
 * `S`, and refresh the receiving route's caches. Returns that route, or NULL,
 * with nothing changed, if no candidate has a feasible position.
 *
 * A weighted criterion measures the change in makespan against every route of
 * `S`, so all of their caches must be fresh, not only the candidates'.
 */
Route *insert_at_best_position(Solution *S, Route **candidates, int n, int *job,
                               const Criterion *c);

#endif
