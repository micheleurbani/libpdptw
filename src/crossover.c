#include <stdlib.h>
#include "crossover.h"
#include "placement.h"

/* Put a copy of `src` in slot `slot` of `S`, and mark the nodes it serves. */
static void inherit(Solution *S, int slot, const Route *src, bool *served) {
    free_route(S->routes[slot]);
    Route *r = copy_route(src);
    r->id = slot;
    update_route(S->problem, r);
    S->routes[slot] = r;
    for (int i = 1; i < r->n - 1; i++) served[r->path[i]] = true;
}

static bool serves_any(const Route *r, const bool *served) {
    for (int i = 1; i < r->n - 1; i++) {
        if (served[r->path[i]]) return true;
    }
    return false;
}

/* Whether `S` places `job` in a route that serves a job, else in its first
 * empty route. */
static bool place(Solution *S, int *job, const Criterion *c, Route **candidates) {
    PDPTW *P = S->problem;
    int n = 0;
    Route *empty = NULL;
    for (int i = 0; i < P->M; i++) {
        Route *t = S->routes[i];
        if (t->njobs > 0) {
            candidates[n++] = t;
        } else if (!empty) {
            empty = t;
        }
    }
    if (insert_at_best_position(S, candidates, n, job, c)) return true;
    return empty && insert_at_best_position(S, &empty, 1, job, c);
}

Solution *crossover(Solution *p, Solution *q, const Criterion *c, Rng *rng) {
    PDPTW *P = p->problem;
    Solution *S = create_solution(P);
    bool *served = calloc(P->N, sizeof(bool));
    int used = 0;  // slots of S that hold an inherited route

    // 1. k of p's n routes that serve a job: a partial Fisher-Yates shuffle
    // makes routes[0..k-1] a uniform sample, copied in p's route order.
    int *routes = malloc(sizeof(int) * P->M);
    int n = 0;
    for (int i = 0; i < P->M; i++) {
        if (p->routes[i]->njobs > 0) routes[n++] = i;
    }
    const int k = n <= 1 ? n : 1 + (int)rng_below(rng, (uint32_t)(n - 1));
    bool *chosen = calloc(P->M, sizeof(bool));
    for (int i = 0; i < k; i++) {
        const int j = i + (int)rng_below(rng, (uint32_t)(n - i));
        const int swap = routes[i];
        routes[i] = routes[j];
        routes[j] = swap;
        chosen[routes[i]] = true;
    }
    for (int i = 0; i < P->M; i++) {
        if (chosen[i]) inherit(S, used++, p->routes[i], served);
    }

    // 2. q's routes that share no job with those, while a slot is free.
    for (int i = 0; i < P->M && used < P->M; i++) {
        const Route *r = q->routes[i];
        if (r->njobs > 0 && !serves_any(r, served)) inherit(S, used++, r, served);
    }

    // 3. The remaining jobs, in random order.
    int **left = malloc(sizeof(int *) * (P->N / 2));
    int n_left = 0;
    for (int j = 0; j < P->N / 2; j++) {
        if (!served[P->jobs[j][0]]) left[n_left++] = P->jobs[j];
    }
    for (int i = n_left - 1; i > 0; i--) {
        const int j = (int)rng_below(rng, (uint32_t)i + 1);
        int *swap = left[i];
        left[i] = left[j];
        left[j] = swap;
    }
    Route **candidates = malloc(sizeof(Route *) * P->M);
    bool placed = true;
    for (int i = 0; i < n_left && placed; i++) placed = place(S, left[i], c, candidates);

    free(candidates);
    free(left);
    free(chosen);
    free(routes);
    free(served);
    if (!placed) {
        free_solution(S);
        return NULL;
    }
    S->n_ujobs = 0;
    count_vehicles(S);
    return S;
}
