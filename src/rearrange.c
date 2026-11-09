#include <stdlib.h>
#include <string.h>
#include "rearrange.h"
#include "insertion.h"

static int position_of(const Route *r, int node) {
    for (int i = 0; i < r->n; i++) {
        if (r->path[i] == node) return i;
    }
    return -1;
}

/*
 * Insert `job` at its best position for `obj` over the `n` candidate routes,
 * and refresh the receiving route's caches. Returns that route, or NULL, with
 * nothing changed, if no candidate has a feasible position.
 */
static Route *insert_at_best_position(PDPTW *P, Route **candidates, int n, int *job,
                                      enum Objective obj) {
    int room = 0;
    for (int k = 0; k < n; k++) {
        const int m = max_insertion_positions(candidates[k]);
        if (m > room) room = m;
    }
    InsertionPosition *positions = malloc(sizeof(InsertionPosition) * room);
    InsertionPosition best;
    Route *target = NULL;
    for (int k = 0; k < n; k++) {
        const int found = insertion_positions(P, candidates[k], job, positions);
        for (int i = 0; i < found; i++) {
            if (!target || better_position(&positions[i], &best, obj)) {
                best = positions[i];
                target = candidates[k];
            }
        }
    }
    free(positions);
    if (target) {
        insert_job(target, job, best.pickup, best.delivery);
        update_route(P, target);
    }
    return target;
}

bool move_to_best_position(Solution *S, Route *r, int *job, enum Objective obj) {
    PDPTW *P = S->problem;
    // insert_job at these positions restores the route as it was.
    const int pickup = position_of(r, job[0]);
    const int delivery = position_of(r, job[1]);
    remove_job(r, job);
    update_route(P, r);

    Route *candidates[2] = {r, NULL};
    int n = 1;
    for (int i = 0; i < P->M && r->n > 2; i++) {
        if (S->routes[i]->n == 2) {
            candidates[n++] = S->routes[i];
            break;
        }
    }
    if (insert_at_best_position(P, candidates, n, job, obj)) return true;
    insert_job(r, job, pickup, delivery);
    update_route(P, r);
    return false;
}

bool random_rearrange(Solution *S, Route *r, enum Objective obj, Rng *rng) {
    int *job = r->jobs[rng_below(rng, (uint32_t)r->njobs)];
    return move_to_best_position(S, r, job, obj);
}

/* Every route that serves a job, and the first empty route, if any. */
static int route_rearrange_candidates(Solution *S, Route **out) {
    int n = 0;
    bool has_empty = false;
    for (int i = 0; i < S->problem->M; i++) {
        Route *t = S->routes[i];
        if (t->njobs > 0) {
            out[n++] = t;
        } else if (!has_empty) {
            out[n++] = t;
            has_empty = true;
        }
    }
    return n;
}

int route_rearrange(Solution *S, Route *r, enum Objective obj, Rng *rng) {
    PDPTW *P = S->problem;
    const int njobs = r->njobs;
    if (njobs == 1) return random_rearrange(S, r, obj, rng) ? 1 : 0;
    const int k = 2 + (int)rng_below(rng, (uint32_t)(njobs - 1));

    // A partial Fisher-Yates shuffle: jobs[0..k-1] is a uniform sample of the
    // route's jobs, in uniformly random order, which is the reinsertion order.
    int **original = malloc(sizeof(int *) * njobs);
    memcpy(original, r->jobs, sizeof(int *) * njobs);
    int **jobs = malloc(sizeof(int *) * njobs);
    memcpy(jobs, r->jobs, sizeof(int *) * njobs);
    for (int i = 0; i < k; i++) {
        const int j = i + (int)rng_below(rng, (uint32_t)(njobs - i));
        int *swap = jobs[i];
        jobs[i] = jobs[j];
        jobs[j] = swap;
    }

    // Remove them, recording the positions that put each one back.
    int *pickup = malloc(sizeof(int) * k);
    int *delivery = malloc(sizeof(int) * k);
    for (int i = 0; i < k; i++) {
        pickup[i] = position_of(r, jobs[i][0]);
        delivery[i] = position_of(r, jobs[i][1]);
        remove_job(r, jobs[i]);
    }
    update_route(P, r);

    Route **candidates = malloc(sizeof(Route *) * P->M);
    Route **placed = malloc(sizeof(Route *) * k);
    int moved = 0;
    for (; moved < k; moved++) {
        const int n = route_rearrange_candidates(S, candidates);
        placed[moved] = insert_at_best_position(P, candidates, n, jobs[moved], obj);
        if (!placed[moved]) break;
    }

    if (moved < k) {
        // Take back the jobs placed so far, then restore r in reverse order of
        // removal, so each recorded position applies to the path it came from.
        for (int i = moved - 1; i >= 0; i--) {
            remove_job(placed[i], jobs[i]);
            update_route(P, placed[i]);
        }
        for (int i = k - 1; i >= 0; i--) insert_job(r, jobs[i], pickup[i], delivery[i]);
        // insert_job appends to the job list; restore its order too, so later
        // draws from r->jobs see the route exactly as it was.
        memcpy(r->jobs, original, sizeof(int *) * njobs);
        update_route(P, r);
        moved = 0;
    }

    free(placed);
    free(candidates);
    free(delivery);
    free(pickup);
    free(jobs);
    free(original);
    return moved;
}
