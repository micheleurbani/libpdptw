/**
 * @file: solution.c
 * @author: Michele Urbani
 *
 * This file contains functions to create and modify a solution to the PDPTW.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include "config.h"
#include "solution.h"
#include "utils.h"

Solution* create_solution(PDPTW* problem) {
    Solution* solution = (Solution*)malloc(sizeof(Solution));
    if (!solution) {
        perror("Failed to allocate solution.");
        exit(EXIT_FAILURE);
    };
    solution->problem = problem;
    solution->Nr = 0;
    solution->n_ujobs = problem->N / 2;
    solution->routes = (Route**)malloc(problem->M * sizeof(Route*));
    if (!solution->routes) {
        perror("Failed to allocate solution routes.");
        exit(EXIT_FAILURE);
    };
    bool done;
    for (size_t i = 0; i < problem->M; i++)
    {
        solution->routes[i] = create_route(i, problem->N + 2);
        done = update_route(problem, solution->routes[i]);
        if (!done)
        {
            printf("Error initializing route %ld.", i);
            exit(1);
        }
    }
    solution->ujobs = (int**)malloc(problem->N / 2 * sizeof(int*));
    if (!solution->ujobs) {
        perror("Failed to allocate list of unassigned jobs.");
        exit(EXIT_FAILURE);
    };
    // A PD-pair is immutable once parsed: `problem->jobs` is written only by
    // read_pdptw_from_file and read everywhere else. Solutions therefore borrow
    // those pointers instead of owning copies of them -- see the ownership note
    // on copy_solution.
    for (size_t i = 0; i < problem->N / 2; i++)
    {
        solution->ujobs[i] = problem->jobs[i];
    }
    return solution;
}

/**
 * Copy a single route, mirroring the allocations of create_route.
 *
 * The `jobs` array holds borrowed pointers into `problem->jobs`, so copying it
 * is a plain memcpy of the pointer array: the pairs themselves are shared with
 * the source route and with every other solution. See copy_solution for why
 * that is safe.
 */
static Route* copy_route(Route* src) {
    Route* r = create_route(src->id, src->capacity);
    r->n = src->n;
    r->njobs = src->njobs;
    memcpy(r->path, src->path, src->capacity * sizeof(int));
    memcpy(r->D, src->D, src->capacity * sizeof(double));
    memcpy(r->T, src->T, src->capacity * sizeof(double));
    memcpy(r->b, src->b, src->capacity * sizeof(double));
    memcpy(r->Q, src->Q, src->capacity * sizeof(int));
    memcpy(r->wait, src->wait, src->capacity * sizeof(double));
    memcpy(r->slack, src->slack, src->capacity * sizeof(double));
    memcpy(r->jobs, src->jobs, (size_t)(src->capacity / 2) * sizeof(int *));
    return r;
}

/**
 * Create an independent copy of a solution. The clone owns its own routes and
 * unassigned-job arrays, so it can be mutated independently, but it shares the
 * read-only problem and the PD-pairs that `problem->jobs` owns.
 *
 * Sharing the pairs is what keeps cloning cheap. A pair is two ints written
 * once by read_pdptw_from_file and never modified afterwards, so no clone can
 * observe another's mutations; the operators move pair *pointers* between the
 * routes and the ujobs list, they never write through them. Owning copies
 * instead would mean one malloc and one free per served job per clone -- on a
 * 1000-node instance the tabu search clones about M^2 solutions per descent
 * sweep, which made that per-pair traffic the dominant cost in both time and
 * memory.
 *
 * The corollary is that neither free_solution nor free_route may free a pair:
 * they are the problem's, and free_problem releases them.
 *
 * @param src The solution to clone.
 * @return A newly allocated Solution; free it with free_solution.
 */
Solution* copy_solution(Solution* src) {
    Solution* s = (Solution*)malloc(sizeof(Solution));
    if (!s) {
        perror("Failed to allocate solution copy.");
        exit(EXIT_FAILURE);
    }
    s->problem = src->problem;
    s->Nr = src->Nr;
    s->n_ujobs = src->n_ujobs;
    s->TC = src->TC;
    s->SD = src->SD;
    s->WT = src->WT;
    s->routes = (Route**)malloc(src->problem->M * sizeof(Route*));
    if (!s->routes) {
        perror("Failed to allocate solution copy routes.");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < src->problem->M; i++)
    {
        s->routes[i] = copy_route(src->routes[i]);
    }
    // The unassigned-job list is allocated for N/2 entries (see create_solution)
    // but only the first n_ujobs are live; the rest are NULL. Borrowed pointers,
    // so the whole array copies at once.
    s->ujobs = (int**)malloc(src->problem->N / 2 * sizeof(int*));
    if (!s->ujobs) {
        perror("Failed to allocate solution copy unassigned jobs.");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ujobs, src->ujobs, (size_t)(src->problem->N / 2) * sizeof(int *));
    return s;
}

void free_solution(Solution* solution) {
    for (size_t i = 0; i < solution->problem->M; i++)
    {
        free_route(solution->routes[i]);
    }
    // The ujobs entries are borrowed from problem->jobs (see copy_solution), so
    // only the array holding them belongs to this solution.
    free(solution->ujobs);
    free(solution->routes);
    // Do not free the problem here. The problem is a shared resource
    // and should be managed separately (e.g., by the C++ Problem wrapper).
    free(solution);
}

void print_solution(Solution* solution) {
    bool is_empty = true;
    printf("Solution:\n");
    for (size_t i = 0; i < solution->problem->M; i++) {
        if (solution->routes[i]->n > 2)
        {
            print_route(solution->routes[i], 0);
            is_empty = false;
        }
    }
    if (is_empty)
    {
        printf("Is empty.\n");
    }
}

bool update_route(PDPTW* P, Route* r) {
    for (int i = 0; i < r->n; i++) {
        if (i > 0) {
            r->D[i] = r->D[i - 1] + P->D[r->path[i - 1]][r->path[i]];
            r->T[i] = r->b[i - 1] + P->s[r->path[i - 1]] +
                P->T[r->path[i - 1]][r->path[i]];
            r->b[i] = fmax(P->e[r->path[i]], r->T[i]);
            r->Q[i] = r->Q[i - 1] + P->q[r->path[i]];
            r->wait[i] = fmax(P->e[r->path[i]] - r->T[i], 0);
            r->slack[i] = fmax(P->l[r->path[i]] - r->T[i], 0);
        } else {
            r->D[i] = 0;
            r->T[i] = 0;
            r->b[i] = 0;
            r->Q[i] = 0;
            r->wait[i] = 0;
            r->slack[i] = 0;
        }
    }
    return true;
}

bool check_route(PDPTW *P, Route *r) {
    bool feasible = true;
    // Update route
    update_route(P, r);
    // Check each node
    for (size_t i = 0; i < r->n; i++)
    {
        // Early TW
        if (r->b[i] < P->e[r->path[i]])
        {
            printf("Service of node %d begins at %lf (<= %lf)\n", r->path[i], r->b[i], P->e[r->path[i]]);
            feasible = false;
        }
        // Late TW
        if (r->b[i] >= P->l[r->path[i]])
        {
            printf("Service of node %d begins at %lf (>= %lf)\n", r->path[i], r->b[i], P->l[r->path[i]]);
            feasible = false;
        }
        // Load
        if (r->Q[i] > P->Q)
        {
            printf("Load at node %d exceeds max load.\n", r->path[i]);
            feasible = false;
        }
    }
    return feasible;
}

/**
 * The function compute the feasibility of routing a job in all admissible
 * positions, and returns the insertion cost defined by `mu` and `alpha`.
 * Let i and j be two consequent nodes and let u be a node that is tentatively
 * inserted between i and j.
 * The distance increase cost is defined as
 * \f[
 *    c_{11} = d_{iu} + d_{uj} - \mu d_{ij}
 * \f]
 *
 * @param P The pointer to the problem object containing all data.
 * @param r The pointer to the route where to try the job insertion.
 * @param job The pointer to the array defining the job to insert.
 * @param[in] mu The value of the `mu` parameter.
 * @param[in] alpha The value of the `alpha` parameter.
 *
 * @return An `MoveCost` object.
 */
MoveCost insertion_cost(PDPTW *P, Route *r, int *job, double mu, double alpha) {
    double b_p = 0;
    double b_d = 0;
    double b_i = 0;
    double b_i_new = 0;
    double PF_i = 0;
    double c12_p = 0;
    double c12_d = 0;
    double c_dist = 0;
    double c_time = 0;
    double c1 = 0;
    double c2 = 0;
    double alpha_ = 1 - alpha;
    int p;
    MoveCost cost = {BIG_M, {0, 0}};
    for (size_t i = 1; i < r->n; i++)
    {
        b_p = fmax(
            P->e[job[0]],
            r->b[i-1] + P->s[r->path[i-1]] + P->T[r->path[i-1]][job[0]]
        );
        if (b_p > P->l[job[0]]) break;
        if (r->Q[i - 1] + P->q[job[0]] > P->Q) continue;
        b_i = fmax(
            P->e[r->path[i]],
            b_p + P->s[job[0]] + P->T[job[0]][r->path[i]]
        );
        if (b_i > P->l[r->path[i]]) continue;
        // Check following nodes
        double bb[r->n - i];
        bb[0] = b_i;
        c12_p = b_i - r->b[i];
        PF_i = c12_p;
        for (size_t j = i + 1; j < r->n; j++)
        {
            if (PF_i > r->wait[j])
            {
                // Wait time is not enough, compute new arrival
                bb[j - i] = fmax(
                    P->e[r->path[j]],
                    bb[j - i - 1] + P->s[r->path[j - 1]] +
                        P->T[r->path[j - 1]][r->path[j]]
                );
                if (bb[j - i] > P->l[r->path[j]]) goto next_pos;
                // Compute new push forward
                PF_i = fmax(0, PF_i - r->wait[j]);
            } else {
                // Wait time is enough
                bb[j - i] = r->b[j];
                PF_i = -BIG_M;
            }
        }

        // Pickup insertion is feasible, compute cost
        c_dist = P->D[r->path[i-1]][job[0]] + P->D[job[0]][r->path[i]] -
            mu * P->D[r->path[i-1]][r->path[i]];
        // c12 = b_ju - b_j, the push forward on the successor caused by
        // inserting the pickup (Solomon eq. 5), not b_i - b_p.
        c_time = c12_p;
        // Pickup insertion is feasible, check delivery insertion
        // Start from position i because it might be the best one
        p = job[0];
        b_i = b_p;
        for (size_t j = i; j < r->n; j++)
        {
            // Compute the arrival time
            b_d = fmax(
                P->e[job[1]],
                b_i + P->s[p] + P->T[p][job[1]]
            );
            // Evaluate late arrival
            if (b_d > P->l[job[1]]) break;
            // Evaluate load only if p != job[0]
            if (p != job[0])
            {
                for (size_t k = j; k > i; k--)
                {
                    if (r->Q[k] + P->q[job[0]] > P->Q) goto stop;
                }
            }
            b_i = fmax(
                P->e[r->path[j]],
                b_d + P->s[job[1]] + P->T[job[1]][r->path[j]]
            );
            if (b_i > P->l[r->path[j]])
            {
                p = r->path[j];
                b_i = bb[j - i];
                continue;
            }
            // Evaluate the shift of the nodes that follow the delivery and make
            // sure none of them is pushed past its late time window. PF_i is the
            // push forward at node j (with both the pickup and the delivery
            // inserted); b_i_new tracks the new begin time as it propagates.
            //
            // At each downstream node k the incoming push (the push at node k-1)
            // is compared against wait[k]: when it does not exceed the available
            // wait it is fully absorbed there, so node k and every later node keep
            // their original begin times and propagation can stop. Otherwise node
            // k's begin time shifts, must be re-validated against its late window,
            // and the residual push (max(0, PF_i - wait[k])) is carried forward.
            c12_d = b_i - r->b[j];
            PF_i = c12_d;
            b_i_new = b_i;
            for (size_t k = j + 1; k < r->n; k++)
            {
                if (PF_i <= r->wait[k])
                {
                    // Push fully absorbed at node k: nothing downstream changes.
                    break;
                }
                b_i_new = fmax(
                    P->e[r->path[k]],
                    b_i_new + P->s[r->path[k-1]] +
                        P->T[r->path[k-1]][r->path[k]]
                );
                if (b_i_new > P->l[r->path[k]])
                {
                    // Infeasible
                    goto stop;
                }
                PF_i = fmax(0, PF_i - r->wait[k]);
            }
            // Calculate insertion cost
            c1 = P->D[p][job[1]] + P->D[job[1]][r->path[j]] -
                mu * P->D[p][r->path[j]];
            // c12 = b_ju - b_j, the push forward on the successor caused by
            // inserting the delivery (Solomon eq. 5), not b_i - b_d.
            c2 = c12_d;
            if (alpha * (c_dist + c1) + alpha_ * (c_time + c2) < cost.cost)
            {
                cost.pos[0] = i;
                cost.pos[1] = j + 1;
                cost.cost = alpha * (c_dist + c1) + alpha_ * (c_time + c2);
                cost.job = job;
            }
            stop:
                b_i = bb[j - i];
                p = r->path[j];
        }
        next_pos:
            ;
    }
    // Check that at least one insertion was evaluated
    if (cost.pos[0] == 0 || cost.pos[1] == 0)
    {
        cost.cost = BIG_M;
        return cost;
    }
    return cost;
}

MoveCost removal_cost(Solution *S, Route *r, int *job, double mu) {
    MoveCost cost = {-BIG_M, {0, 0}, job};
    // Find pickup and delivery position
    cost.pos[0] = find_node_position(r, job[0]);
    cost.pos[1] = find_node_position(r, job[1]);
    cost.cost = S->problem->D[cost.pos[0] - 1][cost.pos[0]]
        + S->problem->D[cost.pos[0]][cost.pos[0] + 1]
        - mu * S->problem->D[cost.pos[0] - 1][cost.pos[0] + 1];
    cost.cost += S->problem->D[cost.pos[1] - 1][cost.pos[1]]
        + S->problem->D[cost.pos[1]][cost.pos[1] + 1]
        - mu * S->problem->D[cost.pos[1] - 1][cost.pos[1] + 1];
    return cost;
}

/**
 * Compute the removal cost of all jobs in the route and return the LARGEST
 * one according to the removal_cost function.
 */
MoveCost worst_cost_removal(Solution *S, Route *r, double mu) {
    if (r->njobs == 0)
    {
        perror("Route is empty.");
    }
    MoveCost cost = {-BIG_M, {0, 0}, NULL};
    MoveCost temp_cost;
    for (size_t i = 0; i < r->njobs; i++)
    {
        temp_cost = removal_cost(S, r, r->jobs[i], mu);
        if (temp_cost.cost > cost.cost)
        {
            cost = temp_cost;
        }
    }
    if (cost.cost == BIG_M || cost.job == NULL)
    {
        perror("No removal cost returned.");
    }
    return cost;
}

/**
 * Remove a job from the solution's unassigned-job list by pointer identity
 * and compact the list in place.
 *
 * @return true if the job was found and removed, false otherwise.
 */
static bool remove_ujob(Solution *S, int *job) {
    int job_id = -1;
    for (size_t i = 0; i < S->n_ujobs; i++)
    {
        if (S->ujobs[i][0] == job[0] && S->ujobs[i][1] == job[1]) {
            job_id = i;
            break;
        }
    }
    if (job_id == -1) {
        return false;
    }
    for (size_t i = job_id; i < S->n_ujobs - 1; i++)
    {
        S->ujobs[i] = S->ujobs[i + 1];
    }
    S->ujobs[S->n_ujobs - 1] = NULL; // Last job is now NULL
    S->n_ujobs--; // Update number of unassigned jobs
    return true;
}

/**
 * Compute the insertion cost in route `route_id` of all unrouted jobs and adds
 * the least cost one to the route. The insertion cost is defined by `mu` and
 * `alpha`.
 *
 * @param[in,out] S A pointer to the solution object.
 * @param[in] P A pointer to the problem data.
 * @param[in] r_id The id of the route to add jobs to.
 * @param[in] mu The value of the `mu` parameter.
 * @param[in] alpha The value of the `alpha` parameter.
 *
 * @pre 0 <= r_id <= M - 1
 *
 * @return Returns true if at least one insertion was possible.
 */
bool insertion_step(Solution *S, int r_id, double mu, double alpha) {
    PDPTW *P = S->problem;
    // No unassigned jobs left: nothing can be inserted. Guard here so the VLA
    // below never has a non-positive bound (undefined behaviour under -O2).
    if (S->n_ujobs <= 0) {
        return false;
    }
    MoveCost insertion_costs[S->n_ujobs];
    for (size_t i = 0; i < S->n_ujobs; i++)
    {
        insertion_costs[i] = insertion_cost(P, S->routes[r_id], S->ujobs[i], mu, alpha);
    }
    // Find the best insertion
    MoveCost best_cost = {BIG_M, {0, 0}, NULL};
    for (size_t i = 0; i < S->n_ujobs; i++)
    {
        if (insertion_costs[i].cost < best_cost.cost)
        {
            best_cost = insertion_costs[i];
        }
    }
    // If no insertion was found, return false
    if (best_cost.cost == BIG_M) {
        return false;
    }
    // Insert the best job
    int success = insert_job(S->routes[r_id], best_cost.job, best_cost.pos[0], best_cost.pos[1]);
    if (!success) {
        return false;
    }
    // Update the route
    success = update_route(P, S->routes[r_id]);
    if (!success) {
        return false;
    }
    if (!remove_ujob(S, best_cost.job)) {
        printf("Job not found in unassigned jobs.\n");
        return false;
    }
    return true;
}

/**
 * Select the PD-pair used to seed a new route, following Li & Lim (2003)
 * §2.2's three simultaneous criteria (an adaptation of Solomon's I1 seed
 * rule to PD-pairs): maximal combined distance to the depot, minimal
 * combined latest time-window bound, and minimal combined time-window
 * width. Neither source specifies how the three combine, so each is
 * min-max normalized over the *feasible* unrouted jobs (feasibility decided
 * by `insertion_cost` against the empty route) and summed with equal
 * weight -- farthest-from-depot added, the two time-window terms
 * subtracted, since smaller is better for those.
 *
 * @param S The solution object.
 * @param r The (empty) route being opened.
 * @param mu The `mu` parameter, forwarded to `insertion_cost`.
 * @param alpha The `alpha` parameter, forwarded to `insertion_cost`.
 * @param[out] out_cost The feasible insertion cost/position of the selected
 * seed job, valid only when the return value is non-NULL.
 *
 * @return A pointer to the selected job in `S->ujobs`, or NULL if no
 * unrouted job can feasibly open the route.
 */
static int *select_seed_job(Solution *S, Route *r, double mu, double alpha, MoveCost *out_cost) {
    PDPTW *P = S->problem;
    if (S->n_ujobs <= 0) {
        return NULL;
    }
    double dist[S->n_ujobs];
    double late[S->n_ujobs];
    double width[S->n_ujobs];
    MoveCost costs[S->n_ujobs];
    bool feasible[S->n_ujobs];

    double dist_min = BIG_M, dist_max = -BIG_M;
    double late_min = BIG_M, late_max = -BIG_M;
    double width_min = BIG_M, width_max = -BIG_M;
    int n_feasible = 0;

    for (size_t i = 0; i < S->n_ujobs; i++)
    {
        int *job = S->ujobs[i];
        costs[i] = insertion_cost(P, r, job, mu, alpha);
        feasible[i] = costs[i].cost < BIG_M;
        if (!feasible[i]) {
            continue;
        }
        n_feasible++;
        dist[i] = P->D[0][job[0]] + P->D[0][job[1]];
        late[i] = P->l[job[0]] + P->l[job[1]];
        width[i] = (P->l[job[0]] - P->e[job[0]]) + (P->l[job[1]] - P->e[job[1]]);
        dist_min = fmin(dist_min, dist[i]);
        dist_max = fmax(dist_max, dist[i]);
        late_min = fmin(late_min, late[i]);
        late_max = fmax(late_max, late[i]);
        width_min = fmin(width_min, width[i]);
        width_max = fmax(width_max, width[i]);
    }
    if (n_feasible == 0) {
        return NULL;
    }
    double dist_range = dist_max - dist_min;
    double late_range = late_max - late_min;
    double width_range = width_max - width_min;

    int best_idx = -1;
    double best_score = -BIG_M;
    for (size_t i = 0; i < S->n_ujobs; i++)
    {
        if (!feasible[i]) {
            continue;
        }
        double dist_n = dist_range > 0 ? (dist[i] - dist_min) / dist_range : 0.0;
        double late_n = late_range > 0 ? (late[i] - late_min) / late_range : 0.0;
        double width_n = width_range > 0 ? (width[i] - width_min) / width_range : 0.0;
        double score = dist_n - late_n - width_n;
        if (score > best_score)
        {
            best_score = score;
            best_idx = i;
        }
    }
    if (best_idx < 0) {
        return NULL;
    }
    if (out_cost != NULL) {
        *out_cost = costs[best_idx];
    }
    return S->ujobs[best_idx];
}

/**
 * The function fills a route with unrouted jobs according to a cost criterion
 * until there are feasible insertions. The route is opened with Li & Lim's
 * seed PD-pair rule (see `select_seed_job`) when it starts empty; the
 * remainder is filled by ordinary min-cost insertion.
 */
bool route_construction(Solution *solution, int route_id, double mu, double alpha)
{
    bool success;
    if (solution->n_ujobs == 0)
    {
        return false;
    }
    Route *r = solution->routes[route_id];
    if (r->njobs == 0)
    {
        MoveCost seed_cost;
        int *seed_job = select_seed_job(solution, r, mu, alpha, &seed_cost);
        if (seed_job != NULL)
        {
            success = insert_job(r, seed_job, seed_cost.pos[0], seed_cost.pos[1]);
            if (success) {
                success = update_route(solution->problem, r);
            }
            if (success) {
                success = remove_ujob(solution, seed_job);
            }
        }
    }
    for (size_t i = 0; i < solution->problem->N / 2; i++)
    {
        success = insertion_step(solution, route_id, mu, alpha);
        if (!success)
        {
            break;
        }
    }
    solution->Nr++;
    return true;
}

/**
 * The function defines a class of insertion heuristics, which add jobs to an
 * empty or partially initialized solution until there are no remainign
 * unrouted jobs, or there are no longer feasible insertion positions.
 * The insertion heuristic is defined by the parameters `mu` and `alpha`.
 *
 * @param[in,out] solution A pointer to an initialized solution.
 * @param[in] problem A pointer to the problem data.
 * @param[in] mu The importance of the original \f$i, j\f$ distance
 * contribution.
 * @param[in] alpha The contribution of the distance cost; it is complementary
 * to time shift cost.
 *
 * @pre 0 <= mu <= 1
 * @pre 0 <= alpha <= 1
 */
void insertion_heuristic(Solution *solution, double mu, double alpha)
{
    bool success;
    for (size_t i = 0; i < solution->problem->M; i++)
    {
        success = route_construction(solution, i, mu, alpha);
        if (!success)
        {
            break;
        }
    }
}

/**
 * Re-order a single route with Solomon's insertion procedure (step 5.3 of the
 * Li & Lim metaheuristic). The route is emptied and its own PD-pairs are
 * re-inserted one by one in their least-cost feasible position.
 *
 * The re-insertion pool is restricted to the jobs the route already served, so
 * the global unassigned-job list is never touched: a re-order can only permute
 * a route's own jobs, never strand them. Greedy re-insertion is not guaranteed
 * to re-fit every job; if it cannot, the route is reverted to its original
 * state and false is returned, so no job is ever dropped.
 *
 * @param S The solution object.
 * @param route_id The id of the route to re-order.
 * @param mu The importance of the original i, j distance contribution.
 * @param alpha The contribution of the distance cost (complementary to time).
 * @return true if the whole route was re-ordered; false if it was reverted.
 */
bool reorder_route(Solution *S, int route_id, double mu, double alpha) {
    PDPTW *P = S->problem;
    Route *r = S->routes[route_id];
    int njobs = r->njobs;
    if (njobs == 0) return true;

    // Snapshot the route so it can be restored if re-insertion is incomplete.
    // orig_jobs stays immutable for the revert; pool is the working copy that is
    // consumed (NULLed) as jobs are re-inserted.
    int orig_n = r->n;
    int orig_path[orig_n];
    memcpy(orig_path, r->path, (size_t)orig_n * sizeof(int));
    int *orig_jobs[njobs];
    memcpy(orig_jobs, r->jobs, (size_t)njobs * sizeof(int *));
    int *pool[njobs];
    memcpy(pool, r->jobs, (size_t)njobs * sizeof(int *));

    // Empty the route, keeping only the depot and the dummy depot.
    r->n = 2;
    r->njobs = 0;
    r->path[0] = 0;
    r->path[1] = 0;
    for (int i = 0; i < njobs; i++) r->jobs[i] = NULL;
    update_route(P, r);

    // Greedily re-insert the route's own jobs (Solomon), cheapest first.
    int inserted = 0;
    bool progress = true;
    while (progress && inserted < njobs)
    {
        progress = false;
        MoveCost best = {BIG_M, {0, 0}, NULL};
        int best_k = -1;
        for (int k = 0; k < njobs; k++)
        {
            if (pool[k] == NULL) continue;
            MoveCost c = insertion_cost(P, r, pool[k], mu, alpha);
            if (c.cost < best.cost)
            {
                best = c;
                best_k = k;
            }
        }
        if (best_k >= 0 && best.cost < BIG_M)
        {
            insert_job(r, pool[best_k], best.pos[0], best.pos[1]);
            update_route(P, r);
            pool[best_k] = NULL;
            inserted++;
            progress = true;
        }
    }

    if (inserted < njobs)
    {
        // Greedy could not re-fit every job: restore the original route so the
        // unplaced pairs are not lost.
        r->n = orig_n;
        memcpy(r->path, orig_path, (size_t)orig_n * sizeof(int));
        memcpy(r->jobs, orig_jobs, (size_t)njobs * sizeof(int *));
        r->njobs = njobs;
        update_route(P, r);
        return false;
    }
    return true;
}

/**
 * The PD-Shift class of operators as defined in Li & Lim. Removes a PD-pair
 * from *Route1* and insert it in the best position in *Route2*.
 */
bool pd_shift(Solution *S, Route *r1, Route *r2, double mu, double alpha)
{
    // Remove from route 1
    MoveCost c1 = worst_cost_removal(S, r1, mu);
    // Check that insertion in route 2 is feasible
    MoveCost c2 = insertion_cost(S->problem, r2, c1.job, mu, alpha);
    if (c2.cost < BIG_M)
    {
        bool success = remove_job(r1, c1.job);
        if (success)
        {
            success = insert_job(r2, c2.job, c2.pos[0], c2.pos[1]);
        }
        if (success) {
            return true;
        }
    }
    return false;
}

/**
 * The PD-Exchange operator.
 *
 * Selects the job with the worst removal cost from r1 and the job with the
 * worst removal cost from r2. It then attempts to swap these jobs: inserting
 * the job from r1 into r2 and the job from r2 into r1.
 *
 * @param S The solution object.
 * @param r1 The first route.
 * @param r2 The second route.
 * @param mu The removal cost parameter.
 * @param alpha The insertion cost parameter.
 * @return true if the exchange was successful, false otherwise.
 */
bool pd_exchange(Solution *S, Route *r1, Route *r2, double mu, double alpha)
{
    // Find PD-pair to remove from route 1
    MoveCost c11 = worst_cost_removal(S, r1, mu);
    bool success1 = remove_job(r1, c11.job);
    success1 = update_route(S->problem, r1);
    // Find PD-pair to remove from route 2
    MoveCost c22 = worst_cost_removal(S, r2, mu);
    bool success2 = remove_job(r2, c22.job);
    success2 = update_route(S->problem, r2);
    // Check that insertion in route 2 is feasible
    MoveCost c12 = insertion_cost(S->problem, r2, c11.job, mu, alpha);
    if (c12.cost >= BIG_M) {
        success1 = insert_job(r1, c11.job, c11.pos[0], c11.pos[1]);
        success1 = update_route(S->problem, r1);
        success2 = insert_job(r2, c22.job, c22.pos[0], c22.pos[1]);
        success2 = update_route(S->problem, r2);
        return false;
    }
    // Check that insertion in route 1 is feasible
    MoveCost c21 = insertion_cost(S->problem, r1, c22.job, mu, alpha);
    if (c21.cost >= BIG_M) {
        success1 = insert_job(r1, c11.job, c11.pos[0], c11.pos[1]);
        success1 = update_route(S->problem, r1);
        success2 = insert_job(r2, c22.job, c22.pos[0], c22.pos[1]);
        success2 = update_route(S->problem, r2);
        return false;
    }
    // PD-Exchange is possible
    success1 = insert_job(r2, c12.job, c12.pos[0], c12.pos[1]);
    success1 = update_route(S->problem, r1);
    success2 = insert_job(r1, c21.job, c21.pos[0], c21.pos[1]);
    success2 = update_route(S->problem, r2);
    if (!success1 || !success2) {
        perror("Insertion failed after PD-Exchange was found feasible.");
        exit(1);
    }
    return true;
}

/**
 * The PD-Rearrange operator.
 *
 * Removes the job with the worst removal cost from the route and re-inserts
 * it into the best position within the same route.
 *
 * @param S The solution object.
 * @param r The route to rearrange.
 * @param mu The removal cost parameter.
 * @param alpha The insertion cost parameter.
 * @return true if the rearrangement was successful, false otherwise.
 */
bool pd_rearrange(Solution *S, Route *r, double mu, double alpha) {
    // Find PD-pair to remove from route
    MoveCost c_removal = worst_cost_removal(S, r, mu);
    bool success = remove_job(r, c_removal.job);
    success = update_route(S->problem, r);
    if (!success) return false;
    MoveCost c_insert = insertion_cost(S->problem, r, c_removal.job, mu, alpha);
    // No feasible re-insertion was found: restore the pair at its original
    // position and report that no move was performed.
    if (c_insert.cost >= BIG_M || c_insert.job == NULL) {
        insert_job(r, c_removal.job, c_removal.pos[0], c_removal.pos[1]);
        update_route(S->problem, r);
        return false;
    }
    success = insert_job(r, c_insert.job, c_insert.pos[0], c_insert.pos[1]);
    if (!success) return false;
    success = update_route(S->problem, r);
    if (!success) return false;
    return true;
}

int save_solution(Solution *solution)
{
    return save_solution_to(solution, NULL);
}

int save_solution_to(Solution *solution, const char *dir)
{
    // Extract the file name from the instance_name path
    const char *path = solution->problem->instance_name;
    const char *basename = strrchr(path, '/');
    if (basename != NULL) {
        basename++; // Move past the '/'
    } else {
        basename = path; // No '/' found
    }

    // Extract instance name (remove extension)
    char instance_name[256];
    strncpy(instance_name, basename, sizeof(instance_name));
    instance_name[sizeof(instance_name) - 1] = '\0';
    char *dot = strrchr(instance_name, '.');
    if (dot) *dot = '\0';

    // Create new file name: instance_name + ".sol", under `dir` when given.
    // A NULL or empty `dir` keeps the historical current-working-directory
    // write, so save_solution() is unchanged for every existing caller.
    char filename[512];
    if (dir != NULL && dir[0] != '\0') {
        size_t len = strlen(dir);
        const char *sep = (dir[len - 1] == '/') ? "" : "/";
        snprintf(filename, sizeof(filename), "%s%s%s.sol", dir, sep, instance_name);
    } else {
        snprintf(filename, sizeof(filename), "%s.sol", instance_name);
    }

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        return 0;
    }

    // Get current date
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char date_str[20];
    strftime(date_str, sizeof(date_str), "%d-%b-%y", &tm);
    for(int i = 0; date_str[i]; i++) date_str[i] = tolower(date_str[i]);

    // Write Header
    fprintf(file, "Instance name : %s\n", instance_name);
    fprintf(file, "Authors       : M. Urbani & F. Pilati\n");
    fprintf(file, "Date          : %s\n", date_str);
    fprintf(file, "Reference     : \n");
    fprintf(file, "Solution\n");

    // Write Routes
    int route_count = 1;
    for (size_t i = 0; i < solution->problem->M; i++)
    {
        Route* r = solution->routes[i];
        if (r->n <= 2) continue;

        fprintf(file, "Route %d :", route_count++);
        for (size_t j = 0; j < r->n; j++)
        {
            int node_id = r->path[j];
            if (node_id != 0) {
                fprintf(file, " %d", node_id);
            }
        }
        fprintf(file, "\n");
    }
    fclose(file);
    return 1;
}

/**
 * Count the non-empty routes (i.e. routes that serve at least one job) and
 * store the result in S->Nr. A route is empty when it only contains the depot
 * and dummy depot (n <= 2).
 */
int count_vehicles(Solution* S) {
    int nv = 0;
    for (size_t i = 0; i < S->problem->M; i++)
    {
        if (S->routes[i]->n > 2) nv++;
    }
    S->Nr = nv;
    return nv;
}

double get_total_travel_cost(Solution* S) {
    double c = 0;
    // Iterate over all routes; empty routes contribute nothing.
    for (size_t i = 0; i < S->problem->M; i++)
    {
        if (S->routes[i]->n <= 2) continue;
        // D[last] = total arc distance of the route (depot → ... → depot).
        c += S->routes[i]->D[S->routes[i]->n - 1];
    }
    return c;
}

double get_total_schedule_duration(Solution* S) {
    double t = 0;
    // Iterate over all routes; empty routes contribute nothing.
    for (size_t i = 0; i < S->problem->M; i++)
    {
        if (S->routes[i]->n <= 2) continue;
        // T[last] = arrival time at the return depot = route schedule duration
        // (travel time + service time + waiting time across all nodes).
        t += S->routes[i]->T[S->routes[i]->n - 1];
    }
    return t;
}

double get_total_waiting_time(Solution* S) {
    double w = 0;
    // Iterate over all routes; empty routes contribute nothing.
    for (size_t i = 0; i < S->problem->M; i++)
    {
        if (S->routes[i]->n <= 2) continue;
        // Get the waiting time before serving node j in route i
        for (size_t j = 0; j < S->routes[i]->n; j++)
        {
            w += S->routes[i]->wait[j];
        }
    }
    return w;
}

double get_solution_cost(Solution* S, double alpha, double beta, double gamma, double lambda) {
    // The solution cost is defiend according to the Li and Lim paper. The
    // penalty weight factors are set to be alpha >> beta >> gamma >> lambda.
    // 1) Number of vehicles (recomputed: non-empty routes may sit anywhere in
    // [0, M) after PD-Shift/Exchange moves, so the stored Nr cannot be trusted).
    int Nr = count_vehicles(S);
    // 2) Travel cost
    S->TC = get_total_travel_cost(S);
    // 3) Total schedule duration
    S->SD = get_total_schedule_duration(S);
    // 4) Total waiting time to start the service
    S->WT = get_total_waiting_time(S);
    // Total cost
    return alpha * Nr + beta * S->TC + gamma * S->SD + lambda * S->WT;
}
