#ifndef ROUTE_H
#define ROUTE_H
#include "config.h"
#include <stdbool.h>

typedef struct Route {
    int id;
    int n;          // Number of nodes in the route (include depot and dummy depot)
    int njobs;      // The number of jobs in the route
    int capacity;   // Allocated length of path/metric arrays
    int *path;      // List of node ids
    double *D;      // Cumulative distance
    double *T;      // Arrival time
    double *b;      // Service begin
    int *Q;         // Cumulative capacity
    double *wait;   // Wait time before to start a job
    double *slack;  // Time before the end of the time windows
    int **jobs;     // The array of jobs in the route
} Route;

Route *create_route(int id, int capacity);
void free_route(Route *route);
void print_route(Route *route, int verbosity);
bool insert_node(Route *route, int id, int pos);
bool insert_job(Route *route, int *job, int pos1, int pos2);
bool remove_node(Route *route, int position);
bool remove_node_by_id(Route *route, int id);
bool remove_job(Route *route, int *job);
int find_node_position(Route *route, int id);

#endif