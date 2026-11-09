#include <stdlib.h>
#include "rearrange.h"
#include "insertion.h"

static int position_of(const Route *r, int node) {
    for (int i = 0; i < r->n; i++) {
        if (r->path[i] == node) return i;
    }
    return -1;
}

bool move_to_best_position(Solution *S, Route *r, int *job, enum Objective obj) {
    PDPTW *P = S->problem;
    // insert_job at these positions restores the route as it was.
    const int pickup = position_of(r, job[0]);
    const int delivery = position_of(r, job[1]);
    remove_job(r, job);
    update_route(P, r);

    Route *candidates[2] = {r, NULL};
    for (int i = 0; i < P->M && r->n > 2; i++) {
        if (S->routes[i]->n == 2) {
            candidates[1] = S->routes[i];
            break;
        }
    }
    // An empty route has a single position, so r's are the most to hold.
    InsertionPosition *positions = malloc(sizeof(InsertionPosition) * max_insertion_positions(r));
    InsertionPosition best;
    Route *target = NULL;
    for (int k = 0; k < 2 && candidates[k]; k++) {
        const int n = insertion_positions(P, candidates[k], job, positions);
        for (int i = 0; i < n; i++) {
            if (!target || better_position(&positions[i], &best, obj)) {
                best = positions[i];
                target = candidates[k];
            }
        }
    }
    free(positions);

    if (!target) {
        insert_job(r, job, pickup, delivery);
        update_route(P, r);
        return false;
    }
    insert_job(target, job, best.pickup, best.delivery);
    update_route(P, target);
    return true;
}

bool random_rearrange(Solution *S, Route *r, enum Objective obj, Rng *rng) {
    int *job = r->jobs[rng_below(rng, (uint32_t)r->njobs)];
    return move_to_best_position(S, r, job, obj);
}
