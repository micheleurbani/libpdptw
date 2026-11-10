#include <math.h>
#include <stdlib.h>
#include "placement.h"

/* The two longest route durations of a solution, over the routes that serve a
 * job, and the route that has the longest. */
typedef struct Makespan {
    double first;
    double second;
    const Route *longest;
} Makespan;

static Makespan makespan_of(const Solution *S) {
    Makespan m = {0, 0, NULL};
    for (int i = 0; i < S->problem->M; i++) {
        const Route *r = S->routes[i];
        if (r->n <= 2) continue;
        const double duration = r->T[r->n - 1];
        if (!m.longest || duration > m.first) {
            m.second = m.first;
            m.first = duration;
            m.longest = r;
        } else if (duration > m.second) {
            m.second = duration;
        }
    }
    return m;
}

/* The change in the weighted sum of the objectives when `job` goes to `p` in
 * route `t`: see Criterion in insertion.h. The new makespan is measured
 * against the other routes, not the solution's makespan, because an insertion
 * can shorten a route when travel times break the triangle inequality. */
static double weighted_change(const InsertionPosition *p, const Route *t, const Makespan *m,
                              const double *weights) {
    const double others = t == m->longest ? m->second : m->first;
    const double delta[NUM_OBJECTIVES] = {
        [OBJ_VEHICLES] = t->n == 2 ? 1 : 0,
        [OBJ_DISTANCE] = p->delta_distance,
        [OBJ_MAKESPAN] = fmax(others, p->duration) - m->first,
        [OBJ_WAITING] = p->delta_waiting,
    };
    double change = 0;
    for (int k = 0; k < NUM_OBJECTIVES; k++) change += weights[k] * delta[k];
    return change;
}

/* Whether position `a`, whose weighted change is `wa`, is better than `b`,
 * whose weighted change is `wb`, for `c`. The weighted changes are read only
 * for a weighted criterion. */
static bool better(const InsertionPosition *a, double wa, const InsertionPosition *b, double wb,
                   const Criterion *c) {
    if (!c->weighted) return better_position(a, b, c->objective);
    if (wa != wb) return wa < wb;
    return a->delta_distance < b->delta_distance;
}

Route *insert_at_best_position(Solution *S, Route **candidates, int n, int *job,
                               const Criterion *c) {
    PDPTW *P = S->problem;
    const Makespan m = c->weighted ? makespan_of(S) : (Makespan){0, 0, NULL};
    int room = 0;
    for (int k = 0; k < n; k++) {
        const int most = max_insertion_positions(candidates[k]);
        if (most > room) room = most;
    }
    InsertionPosition *positions = malloc(sizeof(InsertionPosition) * room);
    InsertionPosition best;
    double best_change = 0;
    Route *target = NULL;
    for (int k = 0; k < n; k++) {
        Route *t = candidates[k];
        const int found = insertion_positions(P, t, job, positions);
        for (int i = 0; i < found; i++) {
            const double change =
                c->weighted ? weighted_change(&positions[i], t, &m, c->weights) : 0;
            if (!target || better(&positions[i], change, &best, best_change, c)) {
                best = positions[i];
                best_change = change;
                target = t;
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
