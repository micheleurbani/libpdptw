#include "construction.h"
#include "config.h"

// Distance-only insertion cost: with mu = alpha = 1, insertion_cost is the
// distance increase of placing the job's pickup and delivery.
#define DISTANCE_MU 1.0
#define DISTANCE_ALPHA 1.0

static void shuffle_jobs(int **jobs, int n, Rng *rng) {
    for (int i = n - 1; i > 0; i--) {
        int j = (int)rng_below(rng, (uint32_t)i + 1);
        int *tmp = jobs[i];
        jobs[i] = jobs[j];
        jobs[j] = tmp;
    }
}

static bool place(PDPTW *P, Route *r, MoveCost c) {
    return insert_job(r, c.job, c.pos[0], c.pos[1]) && update_route(P, r);
}

/*
 * Insert S's unassigned jobs in their current order. Returns false as soon as a
 * job fits in no opened route and no route is left to open.
 */
static bool insert_in_order(Solution *S) {
    PDPTW *P = S->problem;
    int opened = 0;
    for (int k = 0; k < S->n_ujobs; k++) {
        int *job = S->ujobs[k];
        MoveCost best = {BIG_M, {0, 0}, NULL};
        Route *target = NULL;
        for (int i = 0; i < opened; i++) {
            MoveCost c = insertion_cost(P, S->routes[i], job, DISTANCE_MU, DISTANCE_ALPHA);
            if (c.cost < best.cost) {
                best = c;
                target = S->routes[i];
            }
        }
        if (target == NULL) {
            if (opened == P->M) return false;
            target = S->routes[opened++];
            best = insertion_cost(P, target, job, DISTANCE_MU, DISTANCE_ALPHA);
            if (best.cost >= BIG_M) return false;  // the job fits no route at all
        }
        if (!place(P, target, best)) return false;
    }
    S->n_ujobs = 0;
    S->Nr = opened;
    return true;
}

Solution *random_construction(PDPTW *P, Rng *rng, int max_retries) {
    for (int attempt = 0; attempt <= max_retries; attempt++) {
        Solution *S = create_solution(P);
        shuffle_jobs(S->ujobs, S->n_ujobs, rng);
        if (insert_in_order(S)) return S;
        free_solution(S);
    }
    Solution *S = create_solution(P);
    insertion_heuristic(S, RANDOM_CONSTRUCTION_FALLBACK_MU, RANDOM_CONSTRUCTION_FALLBACK_ALPHA);
    return S;
}
