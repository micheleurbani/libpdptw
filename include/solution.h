#ifndef SOLUTION_H
#define SOLUTION_H
#include <stdbool.h>
#include <string.h>
#include "route.h"
#include "problem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MoveCost
{
    double cost;
    int pos[2];
    int *job;
} MoveCost;


typedef struct Solution
{
    PDPTW *problem;     // Pointer to problem object
    Route **routes;     // Array of routes
    int Nr;             // The number of non empty routes
    int **ujobs;        // Array of unasigned jobs
    int n_ujobs;        // Number of unassigned jobs
    double TC;          // The total travel cost
    double SD;          // Total schedule duration
    double WT;          // Total waiting time
} Solution;


Solution *create_solution(PDPTW *problem);

Solution *copy_solution(Solution *src);

void free_solution(Solution *solution);

void print_solution(Solution *solution);

bool update_route(PDPTW *problem, Route *route);

bool check_route(PDPTW *problem, Route *route);

MoveCost insertion_cost(PDPTW *problem, Route *route, int *job, double mu, double alpha);

bool insertion_step(Solution *solution, int route_id, double mu, double alpha);

bool route_construction(Solution *solution, int route_id, double mu, double alpha);

void insertion_heuristic(Solution *solution, double mu, double alpha);

bool reorder_route(Solution *solution, int route_id, double mu, double alpha);

int count_vehicles(Solution *solution);

MoveCost removal_cost(Solution *S, Route *r, int *job, double mu);

MoveCost worst_cost_removal(Solution *S, Route *r, double mu);

bool pd_shift(Solution *solution, Route *r1, Route *r2, double mu, double alpha);

bool pd_exchange(Solution *solution, Route *r1, Route *r2, double mu, double alpha);

bool pd_rearrange(Solution *solution, Route *r1, double mu, double alpha);

double get_total_travel_cost(Solution* S);

double get_total_schedule_duration(Solution* S);

double get_total_waiting_time(Solution* S);

double get_solution_cost(Solution* S, double alpha, double beta, double gamma, double lambda);

int save_solution(Solution *solution);

/* As save_solution, but writes <instance>.sol into `dir` instead of the current
 * working directory. `dir` must already exist; NULL or "" means the CWD, which
 * is exactly what save_solution() passes. */
int save_solution_to(Solution *solution, const char *dir);

#ifdef __cplusplus
}
#endif

#endif // End of SOLUTION_H