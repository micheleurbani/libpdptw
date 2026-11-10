#include <stdlib.h>
#include <string.h>
#include "rearrange.h"
#include "insertion.h"
#include "placement.h"

static int position_of(const Route *r, int node) {
    for (int i = 0; i < r->n; i++) {
        if (r->path[i] == node) return i;
    }
    return -1;
}

bool move_to_best_position(Solution *S, Route *r, int *job, const Criterion *c) {
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
    if (insert_at_best_position(S, candidates, n, job, c)) return true;
    insert_job(r, job, pickup, delivery);
    update_route(P, r);
    return false;
}

bool random_rearrange(Solution *S, Route *r, const Criterion *c, Rng *rng) {
    int *job = r->jobs[rng_below(rng, (uint32_t)r->njobs)];
    return move_to_best_position(S, r, job, c);
}

/* Whether a job being relocated may go to an empty route. */
enum Opening { NO_NEW_ROUTE, MAY_OPEN_ROUTE };

/* Every route that serves a job and, if `opening` allows, the first empty
 * route. */
static int relocation_candidates(Solution *S, enum Opening opening, Route **out) {
    int n = 0;
    bool has_empty = false;
    for (int i = 0; i < S->problem->M; i++) {
        Route *t = S->routes[i];
        if (t->njobs > 0) {
            out[n++] = t;
        } else if (opening == MAY_OPEN_ROUTE && !has_empty) {
            out[n++] = t;
            has_empty = true;
        }
    }
    return n;
}

/*
 * Remove jobs[0..k-1] from `r`, then insert them one at a time, in that
 * order, each at its best position for `c` over every route that serves a
 * job and, if `opening` allows, the first empty route; the routes are re-read before
 * each insertion. If some job has no feasible position, every job placed so
 * far is taken back and `r` is restored as it was, paths and job list alike.
 * Returns whether every job was placed.
 */
static bool relocate_jobs(Solution *S, Route *r, int **jobs, int k, const Criterion *c,
                          enum Opening opening) {
    PDPTW *P = S->problem;
    const int njobs = r->njobs;
    int **original = malloc(sizeof(int *) * njobs);
    memcpy(original, r->jobs, sizeof(int *) * njobs);

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
        const int n = relocation_candidates(S, opening, candidates);
        placed[moved] = insert_at_best_position(S, candidates, n, jobs[moved], c);
        if (!placed[moved]) break;
    }

    const bool all = moved == k;
    if (!all) {
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
    }

    free(placed);
    free(candidates);
    free(delivery);
    free(pickup);
    free(original);
    return all;
}

int route_rearrange(Solution *S, Route *r, const Criterion *c, Rng *rng) {
    const int njobs = r->njobs;
    if (njobs == 1) return random_rearrange(S, r, c, rng) ? 1 : 0;
    const int k = 2 + (int)rng_below(rng, (uint32_t)(njobs - 1));

    // A partial Fisher-Yates shuffle: jobs[0..k-1] is a uniform sample of the
    // route's jobs, in uniformly random order, which is the reinsertion order.
    int **jobs = malloc(sizeof(int *) * njobs);
    memcpy(jobs, r->jobs, sizeof(int *) * njobs);
    for (int i = 0; i < k; i++) {
        const int j = i + (int)rng_below(rng, (uint32_t)(njobs - i));
        int *swap = jobs[i];
        jobs[i] = jobs[j];
        jobs[j] = swap;
    }
    const bool moved = relocate_jobs(S, r, jobs, k, c, MAY_OPEN_ROUTE);
    free(jobs);
    return moved ? k : 0;
}

/* The first route that serves the fewest jobs, or NULL if fewer than two
 * routes serve one, so no route has another to be emptied into. */
static Route *fewest_jobs_route(Solution *S) {
    Route *fewest = NULL;
    int used = 0;
    for (int i = 0; i < S->problem->M; i++) {
        Route *t = S->routes[i];
        if (t->njobs == 0) continue;
        used++;
        if (!fewest || t->njobs < fewest->njobs) fewest = t;
    }
    return used >= 2 ? fewest : NULL;
}

int vehicle_reduction(Solution *S) {
    const Criterion least_duration = objective_criterion(OBJ_VEHICLES);
    int emptied = 0;
    Route *r;
    while ((r = fewest_jobs_route(S))) {
        int **jobs = malloc(sizeof(int *) * r->njobs);
        memcpy(jobs, r->jobs, sizeof(int *) * r->njobs);
        const bool moved = relocate_jobs(S, r, jobs, r->njobs, &least_duration, NO_NEW_ROUTE);
        free(jobs);
        if (!moved) break;
        emptied++;
    }
    return emptied;
}
