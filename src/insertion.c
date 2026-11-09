#include <math.h>
#include "insertion.h"

/* A vehicle's state after serving a node: what update_route caches, plus the
 * waiting time accumulated so far. */
typedef struct Visit {
    int node;
    double arrival;
    double begin;
    int load;
    double waiting;
} Visit;

/* Drive from v's node to `node` and serve it. Returns false if service would
 * begin after the window closes or the load would exceed the capacity. */
static bool visit(PDPTW *P, Visit *v, int node) {
    double arrival = v->begin + P->s[v->node] + P->T[v->node][node];
    double begin = fmax(P->e[node], arrival);
    int load = v->load + P->q[node];
    if (begin > P->l[node] || load > P->Q) return false;
    v->waiting += fmax(P->e[node] - arrival, 0);
    v->node = node;
    v->arrival = arrival;
    v->begin = begin;
    v->load = load;
    return true;
}

int max_insertion_positions(const Route *r) {
    return r->n * (r->n - 1) / 2;
}

int insertion_positions(PDPTW *P, Route *r, int *job, InsertionPosition *out) {
    const int pick = job[0];
    const int deliv = job[1];
    const int *path = r->path;
    double **D = P->D;
    double waiting_before = 0;
    for (int i = 0; i < r->n; i++) waiting_before += r->wait[i];

    int count = 0;
    double prefix_waiting = 0;  // waiting at path[0..p-1]
    for (int p = 1; p < r->n; p++) {
        prefix_waiting += r->wait[p - 1];
        Visit after_pickup = {path[p - 1], r->T[p - 1], r->b[p - 1], r->Q[p - 1], prefix_waiting};
        if (!visit(P, &after_pickup, pick)) continue;
        const double pick_delta = D[path[p - 1]][pick] + D[pick][path[p]] - D[path[p - 1]][path[p]];
        // The delivery goes right before path[k]; `after_pickup` has served
        // path[p..k-1] with the job on board.
        for (int k = p; k < r->n; k++) {
            Visit v = after_pickup;
            bool ok = visit(P, &v, deliv);
            for (int i = k; ok && i < r->n; i++) ok = visit(P, &v, path[i]);
            if (ok) {
                double delta;
                if (k == p) {  // pickup and delivery split the same arc
                    delta = D[path[p - 1]][pick] + D[pick][deliv] + D[deliv][path[p]] -
                            D[path[p - 1]][path[p]];
                } else {
                    delta = pick_delta + D[path[k - 1]][deliv] + D[deliv][path[k]] -
                            D[path[k - 1]][path[k]];
                }
                out[count++] = (InsertionPosition){
                    p, k + 1, delta, v.arrival, v.waiting - waiting_before};
            }
            if (!visit(P, &after_pickup, path[k])) break;
        }
    }
    return count;
}

bool better_position(const InsertionPosition *a, const InsertionPosition *b,
                     enum Objective obj) {
    double fa, fb;
    switch (obj) {
        case OBJ_MAKESPAN: fa = a->duration; fb = b->duration; break;
        case OBJ_WAITING: fa = a->delta_waiting; fb = b->delta_waiting; break;
        case OBJ_DISTANCE:
        case OBJ_VEHICLES:
        default: fa = a->delta_distance; fb = b->delta_distance; break;
    }
    if (fa != fb) return fa < fb;
    return a->delta_distance < b->delta_distance;
}
