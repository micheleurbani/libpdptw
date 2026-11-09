#include <math.h>
#include "objectives.h"

ObjectiveVector evaluate_objectives(Solution *S) {
    PDPTW *P = S->problem;
    double makespan = 0;
    for (int i = 0; i < P->M; i++) {
        Route *r = S->routes[i];
        update_route(P, r);
        // T at the closing depot is the arrival time back at the depot.
        if (r->n > 2) makespan = fmax(makespan, r->T[r->n - 1]);
    }
    ObjectiveVector v;
    v.f[OBJ_VEHICLES] = count_vehicles(S);
    v.f[OBJ_DISTANCE] = get_total_travel_cost(S);
    v.f[OBJ_MAKESPAN] = makespan;
    v.f[OBJ_WAITING] = get_total_waiting_time(S);
    return v;
}
