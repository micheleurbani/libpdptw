#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "route.h"
#include "problem.h"

/*
 * A route's arrays share one block, laid out as the five double caches (D, T,
 * b, wait, slack), then `capacity / 2` job pointers, then the two int arrays
 * (path, Q), each `capacity` long. D comes first, so it is the block's address.
 */
static size_t route_storage_size(int capacity) {
    return (size_t)capacity * (5 * sizeof(double) + 2 * sizeof(int)) +
           (size_t)(capacity / 2) * sizeof(int *);
}

static void *allocate_route_storage(int capacity, bool zeroed) {
    void *block = zeroed ? calloc(1, route_storage_size(capacity))
                         : malloc(route_storage_size(capacity));
    if (!block) {
        perror("Failed to allocate route storage.");
        exit(EXIT_FAILURE);
    }
    return block;
}

/* Point r's arrays into `block`, which holds `capacity` entries. */
static void attach_route_storage(Route *r, void *block, int capacity) {
    double *d = block;
    r->D = d;
    r->T = d + capacity;
    r->b = d + 2 * capacity;
    r->wait = d + 3 * capacity;
    r->slack = d + 4 * capacity;
    r->jobs = (int **)(d + 5 * capacity);
    int *i = (int *)(r->jobs + capacity / 2);
    r->path = i;
    r->Q = i + capacity;
    r->capacity = capacity;
}

/* Give dst fresh storage for `capacity` nodes holding src's live entries --
 * n path and cache entries, njobs jobs -- and nothing else. dst's counts must
 * already match src's, and capacity must be at least src->n. */
static void store_live_entries(Route *dst, const Route *src, int capacity) {
    attach_route_storage(dst, allocate_route_storage(capacity, false), capacity);
    const size_t n = (size_t)src->n;
    memcpy(dst->path, src->path, n * sizeof(int));
    memcpy(dst->Q, src->Q, n * sizeof(int));
    memcpy(dst->D, src->D, n * sizeof(double));
    memcpy(dst->T, src->T, n * sizeof(double));
    memcpy(dst->b, src->b, n * sizeof(double));
    memcpy(dst->wait, src->wait, n * sizeof(double));
    memcpy(dst->slack, src->slack, n * sizeof(double));
    memcpy(dst->jobs, src->jobs, (size_t)src->njobs * sizeof(int *));
}

static Route *allocate_route(void) {
    Route *r = malloc(sizeof(Route));
    if (!r) {
        perror("Failed to allocate route.");
        exit(EXIT_FAILURE);
    }
    return r;
}

Route* create_route(int id, int capacity) {
    Route* new_route = allocate_route();
    new_route->id = id;
    new_route->n = 2;
    new_route->njobs = 0;
    // Zeroed: path is all depots, every cache 0 and every job pointer NULL.
    attach_route_storage(new_route, allocate_route_storage(capacity, true), capacity);
    return new_route;
}

Route *copy_route(const Route *src) {
    Route *r = allocate_route();
    r->id = src->id;
    r->n = src->n;
    r->njobs = src->njobs;
    // Room for one more job, so a single insertion doesn't grow the copy.
    store_live_entries(r, src, src->n + 2);
    return r;
}

void free_route(Route* route) {
    free(route->D);  // the start of the route's storage block
    free(route);
}

void print_route(Route* route, int verbosity) {
    printf("Route %d:", route->id);
    for (size_t i = 0; i < route->n; i++) {
        if (route->path[i] == -1) {
            printf("Check n_nodes.");
            break;
        }
        if (i == 0) {
            printf(" %d", route->path[i]);
        } else {
            printf(" -> (%ld)%d", i, route->path[i]);
        }
    }
    printf("\n");
    if (verbosity == 1)
    {
        printf("%5s %5s %10s %10s %10s %5s %10s %10s\n",
            "Idx", "ID", "D", "T", "b", "Q", "Wait", "Slack");
        for (int i = 0; i < route->n; i++) {
            printf("%5d %5d %10.3lf %10.3lf %10.3lf %5d %10.3lf %10.3lf\n",
                i, route->path[i], route->D[i], route->T[i], route->b[i],
                route->Q[i], route->wait[i], route->slack[i]);
        }
    }
    printf("\n");
}

/* Make room for at least one more node, doubling the capacity if the route is
 * full. This moves the route's arrays, so pointers into them go stale. */
static void reserve_one_more_node(Route *r) {
    if (r->n < r->capacity) return;
    Route grown = *r;
    store_live_entries(&grown, r, 2 * r->capacity);
    free(r->D);
    *r = grown;
}

bool insert_node(Route* r, int id, int pos) {
    if (pos == 0 || pos == r->n) {
        printf("Cannot move the depot.\n");
        return false;
    }
    reserve_one_more_node(r);
    for (size_t i = r->n; i > pos; i--)
    {
        r->path[i] = r->path[i - 1];
    }
    r->path[pos] = id;
    r->n++;
    return true;
}

/**
 * Inserts a job in a route.
 *
 * @param route The route to modify.
 * @param job The job to insert.
 * @param pos1 The position of the pickup node.
 * @param pos2 The position of the delivery node.
 * @return true if the job was successfully inserted, false otherwise.
 */
bool insert_job(Route *route, int *job, int pos1, int pos2)
{
    // Insert the best job
    bool success = insert_node(route, job[0], pos1);
    if (!success) {
        return false;
    }
    success = insert_node(route, job[1], pos2);
    if (!success) {
        return false;
    }
    // Add the job to the route
    route->jobs[route->njobs] = job;
    // Increase job number
    route->njobs++;
    return true;
}

bool remove_node(Route* r, int pos) {
    if (pos >= r->n - 1 || pos == 0) {
        printf("Cannot remove the depot.\n");
        return false;
    }
    for (size_t i = pos; i < r->n - 1; i++)
    {
        r->path[i] = r->path[i+1];
    }
    r->n--;
    return true;
}

/**
 * @brief Remove a node from the solution using its ID.
 *
 * @param route The pointer to the route to search.
 * @param id The ID of the node to find.
 * @return true if the node was successfully removed, false otherwise.
 */
bool remove_node_by_id(Route* r, int id) {
    if (id == 0)
    {
        printf("Cannot remove the depot.\n");
        return false;
    }
    int advance_node = 0;
    // The last node is the dummy depot, never `id`, so it is only shifted.
    for (size_t i = 0; i < r->n - 1; i++)
    {
        if (r->path[i] == id || advance_node)
        {
            r->path[i] = r->path[i+1];
            advance_node = 1;
        }
    }
    if (!advance_node) {
        printf("Node %d not found in route %d.\n", id, r->id);
        return false;
    }
    r->n--;
    return true;
}

/**
 * Removes a job from the route.
 *
 * Removes the pickup and delivery nodes associated with the job from the route's path
 * and updates the internal list of jobs.
 *
 * @param route The pointer to the route to modify.
 * @param job The pointer to the job to remove.
 * @return true if the job was successfully removed, false otherwise.
 */
bool remove_job(Route *route, int *job)
{
    int success;
    // Remove pickup
    success = remove_node_by_id(route, job[0]);
    if (!success) {
        printf("Node %d is not present in route %d.\n", job[0], route->id);
        return false;
    }
    // Remove delivery
    success = remove_node_by_id(route, job[1]);
    if (!success) {
        printf("Node %d is not present in route %d.\n", job[1], route->id);
        return false;
    }

    // Remove the job from the route's job list to maintain consistency
    int job_idx = -1;
    for (int i = 0; i < route->njobs; i++) {
        if (route->jobs[i] == job) {
            job_idx = i;
            break;
        }
    }
    if (job_idx != -1) {
        for (int i = job_idx; i < route->njobs - 1; i++) {
            route->jobs[i] = route->jobs[i + 1];
        }
        route->njobs--;
    }

    return true;
}

/**
 * Finds the position of a node in the route.
 *
 * @param route The pointer to the route to search.
 * @param id The ID of the node to find.
 * @return The index of the node in the path, or -1 if not found.
 */
int find_node_position(Route *route, int id) {
    for (size_t i = 0; i < route->n; i++)
    {
        if (route->path[i] == id) return i;
    }
    printf("Node %d not in route %d.\n", id, route->id);
    return -1;
}
