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
    double delta_duration;  // the route's duration after minus before
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
 * - OBJ_VEHICLES: smaller Δduration, the route duration the job adds;
 * with ties going to the smaller Δdistance.
 */
bool better_position(const InsertionPosition *a, const InsertionPosition *b,
                     enum Objective obj);

/**
 * What a best position is judged on.
 *
 * - One objective (`weighted` false): better_position for `objective`.
 * - A weighted sum (`weighted` true): the position whose insertion changes
 *   the sum over objectives of weights[k] * f_k the least, where each weight
 *   is per unit of its raw objective. Δf1 is 1 if the receiving route served
 *   no job, Δf2 is Δdistance, Δf3 is the change in the solution's longest
 *   route duration and Δf4 is Δwaiting. Ties go to the smaller Δdistance.
 *
 * Build one with objective_criterion or weighted_criterion.
 */
typedef struct Criterion {
    bool weighted;
    enum Objective objective;        // when not weighted
    double weights[NUM_OBJECTIVES];  // when weighted
} Criterion;

/** Judge positions on objective `obj` alone. */
Criterion objective_criterion(enum Objective obj);

/** Judge positions on the weighted sum with `weights`, one per unit of each
 *  raw objective, indexed by enum Objective. */
Criterion weighted_criterion(const double weights[NUM_OBJECTIVES]);

#ifdef __cplusplus
}
#endif

#endif
