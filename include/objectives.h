#ifndef OBJECTIVES_H
#define OBJECTIVES_H
#include "solution.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Indices into ObjectiveVector.f. Every objective is minimised. */
enum Objective {
    OBJ_VEHICLES = 0,   // f1: number of non-empty routes
    OBJ_DISTANCE,       // f2: total distance over all routes
    OBJ_MAKESPAN,       // f3: longest route duration
    OBJ_WAITING,        // f4: total waiting time over all nodes
    NUM_OBJECTIVES
};

typedef struct ObjectiveVector {
    double f[NUM_OBJECTIVES];
} ObjectiveVector;

/**
 * Compute the objective vector (f1, f2, f3, f4) of a solution.
 *
 * Every route's caches are refreshed first, so the result is valid right
 * after structural operators. Vehicles leave the depot at time 0, so a route's
 * duration is its arrival time back at the depot, and waiting at the first
 * node counts towards both the duration and f4.
 *
 * Feasibility is not checked: call check_route on the routes for that.
 */
ObjectiveVector evaluate_objectives(Solution *S);

#ifdef __cplusplus
}
#endif

#endif
