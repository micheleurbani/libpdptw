#include <stdio.h>
#include <stdlib.h>
#include "route.h"
#include "problem.h"

Route* create_route(int id, int capacity) {
    Route* new_route = (Route*)malloc(sizeof(Route));
    if (!new_route) {
        perror("Failed to allocate route.");
        exit(EXIT_FAILURE);
    };
    new_route->id = id;
    new_route->n = 2;
    new_route->njobs = 0;
    new_route->capacity = capacity;
    new_route->path = (int*)calloc(capacity, sizeof(int));
    if (!new_route->path) {
        perror("Failed to allocate space for node pointers.");
        exit(EXIT_FAILURE);
    }
    new_route->path[0] = 0;
    new_route->path[1] = 0;
    new_route->D = (double*)calloc(capacity, sizeof(double));
    if (!new_route->D) {
        perror("Failed to allocate route distance.");
        exit(EXIT_FAILURE);
    };
    new_route->T = (double*)calloc(capacity, sizeof(double));
    if (!new_route->T) {
        perror("Failed to allocate route time.");
        exit(EXIT_FAILURE);
    };
    new_route->b = (double*)calloc(capacity, sizeof(double));
    if (!new_route->b) {
        perror("Failed to allocate begin activity time.");
        exit(EXIT_FAILURE);
    };
    new_route->Q = (int*)calloc(capacity, sizeof(int));
    if (!new_route->Q) {
        perror("Failed to allocate route load.");
        exit(EXIT_FAILURE);
    };
    new_route->wait = (double*)calloc(capacity, sizeof(double));
    if (!new_route->wait) {
        perror("Failed to allocate route wait time.");
        exit(EXIT_FAILURE);
    };
    new_route->slack = (double*)calloc(capacity, sizeof(double));
    if (!new_route->slack) {
        perror("Failed to allocate route slack array.");
        exit(EXIT_FAILURE);
    };
    new_route->jobs = (int**)malloc(capacity / 2 * sizeof(int*));
    if (!new_route->jobs) {
        perror("Failed to allocate space for jobs pointers.");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < (size_t)(capacity / 2); i++)
    {
        new_route->jobs[i] = NULL;
    }
    return new_route;
}

void free_route(Route* route) {
    free(route->path);
    free(route->D);
    free(route->T);
    free(route->b);
    free(route->Q);
    free(route->wait);
    free(route->slack);
    free(route->jobs);
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

bool insert_node(Route* r, int id, int pos) {
    if (pos == 0 || pos == r->n) {
        printf("Cannot move the depot.\n");
        return false;
    }
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
    for (size_t i = pos; i < r->n; i++)
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
    for (size_t i = 0; i < r->n; i++)
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
