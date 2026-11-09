#ifndef INSERTION_H
#define INSERTION_H
#include "problem.h"
#include "solution.h"
#include "objectives.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * One feasible way to insert a job into a route, and what it does to the
 * route. `pickup` and `delivery` are the positions insert_job takes: the
 * pickup goes before path[pickup], and the delivery goes at index `delivery`
 * of the path that already holds the pickup.
 */
typedef struct InsertionPosition {
    int pickup;
    int delivery;
    double delta_distance;  // the route's distance after minus before
    double duration;        // the route's duration after the insertion
    double delta_waiting;   // the route's total waiting time after minus before
} InsertionPosition;

/** The most positions insertion_positions can report for route `r`. */
int max_insertion_positions(const Route *r);

/**
 * Report every position of `job` in `r` that keeps the route feasible: no
 * service begins after its time window closes and no load exceeds the
 * vehicle's capacity. Durations and waiting times follow evaluate_objectives:
 * the vehicle leaves the depot at time 0 and a route's duration is its
 * arrival time back at the depot.
 *
 * @pre `r`'s caches are fresh (update_route) and `r` is feasible.
 * @param out Room for at least max_insertion_positions(r) entries.
 * @return The number of positions written to `out`.
 */
int insertion_positions(PDPTW *P, Route *r, int *job, InsertionPosition *out);

/**
 * Whether `a` is a better position than `b` for objective `obj`:
 * - OBJ_DISTANCE: smaller Δdistance;
 * - OBJ_MAKESPAN: shorter duration of the route that receives the job;
 * - OBJ_WAITING:  smaller Δwaiting;
 * with ties going to the smaller Δdistance. OBJ_VEHICLES compares Δdistance.
 */
bool better_position(const InsertionPosition *a, const InsertionPosition *b,
                     enum Objective obj);

#ifdef __cplusplus
}
#endif

#endif
