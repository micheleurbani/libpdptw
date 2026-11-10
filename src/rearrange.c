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

static int index_of(const Route *r, const int *job) {
    for (int i = 0; i < r->njobs; i++) {
        if (r->jobs[i] == job) return i;
    }
    return -1;
}

/* A job taken out of a route, and what puts it back exactly: the positions of
 * its pickup and delivery in the path, and its index in the job list. */
typedef struct Removal {
    Route *route;
    int *job;
    int pickup;
    int delivery;
    int index;
} Removal;

/* Take `job` out of `r`, which serves it. r's caches are left stale. */
static Removal take_out(Route *r, int *job) {
    const Removal x = {r, job, position_of(r, job[0]), position_of(r, job[1]), index_of(r, job)};
    remove_job(r, job);
    return x;
}

/* Put a job back where take_out took it from, in the path and the job list
 * alike. Removals must be put back in reverse order of take_out, so each one
 * applies to the route as it was when the job left. The route's caches are
 * left stale. */
static void put_back(const Removal *x) {
    Route *r = x->route;
    insert_job(r, x->job, x->pickup, x->delivery);
    // insert_job appends the job to the list, and may have moved the list.
    memmove(&r->jobs[x->index + 1], &r->jobs[x->index],
            sizeof(int *) * (size_t)(r->njobs - 1 - x->index));
    r->jobs[x->index] = x->job;
}

/* Take back a move that failed part-way: take each of the first `moved` jobs
 * of `removed` out of the route that received it, placed[i], then put all `k`
 * removals back, in reverse order. Every route involved is as it was before
 * the move, paths and job lists alike, and its caches are fresh. */
static void take_back(PDPTW *P, const Removal *removed, int k, Route **placed, int moved) {
    for (int i = moved - 1; i >= 0; i--) {
        remove_job(placed[i], removed[i].job);
        update_route(P, placed[i]);
    }
    for (int i = k - 1; i >= 0; i--) put_back(&removed[i]);
    // Refresh each run of removals from one route once.
    for (int i = 0; i < k; i++) {
        if (i == 0 || removed[i].route != removed[i - 1].route) {
            update_route(P, removed[i].route);
        }
    }
}

bool move_to_best_position(Solution *S, Route *r, int *job, const Criterion *c) {
    PDPTW *P = S->problem;
    const Removal removed = take_out(r, job);
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
    take_back(P, &removed, 1, NULL, 0);
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
 * job and, if `opening` allows, the first empty route; the routes are re-read
 * before each insertion. If some job has no feasible position, the move is
 * taken back (see take_back). Returns whether every job was placed.
 */
static bool relocate_jobs(Solution *S, Route *r, int **jobs, int k, const Criterion *c,
                          enum Opening opening) {
    PDPTW *P = S->problem;
    Removal *removed = malloc(sizeof(Removal) * k);
    for (int i = 0; i < k; i++) removed[i] = take_out(r, jobs[i]);
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
    if (!all) take_back(P, removed, k, placed, moved);

    free(placed);
    free(candidates);
    free(removed);
    return all;
}

/*
 * Take jobs[i] out of from[i], for every i < k, then insert them one at a
 * time, in that order, each at its best position for `c` in to[i]. If some
 * job has no feasible position, the move is taken back (see take_back).
 * Returns whether every job was placed.
 */
static bool relocate_into(Solution *S, Route **from, int **jobs, Route **to, int k,
                          const Criterion *c) {
    PDPTW *P = S->problem;
    Removal removed[k];
    Route *placed[k];
    for (int i = 0; i < k; i++) removed[i] = take_out(from[i], jobs[i]);
    for (int i = 0; i < k; i++) update_route(P, from[i]);
    int moved = 0;
    for (; moved < k; moved++) {
        placed[moved] = insert_at_best_position(S, &to[moved], 1, jobs[moved], c);
        if (!placed[moved]) break;
    }
    const bool all = moved == k;
    if (!all) take_back(P, removed, k, placed, moved);
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

bool pd_shift(Solution *S, Route *r1, Route *r2, const Criterion *c, Rng *rng) {
    if (r1 == r2 || r1->njobs == 0) return false;
    int *job = r1->jobs[rng_below(rng, (uint32_t)r1->njobs)];
    return relocate_into(S, &r1, &job, &r2, 1, c);
}

bool pd_exchange(Solution *S, Route *r1, Route *r2, const Criterion *c, Rng *rng) {
    if (r1 == r2 || r1->njobs == 0 || r2->njobs == 0) return false;
    int *jobs[2];
    jobs[0] = r1->jobs[rng_below(rng, (uint32_t)r1->njobs)];
    jobs[1] = r2->jobs[rng_below(rng, (uint32_t)r2->njobs)];
    Route *from[2] = {r1, r2};
    Route *to[2] = {r2, r1};
    return relocate_into(S, from, jobs, to, 2, c);
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
