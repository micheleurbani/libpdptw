#ifndef PROBLEM_H
#define PROBLEM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PDPTW {
    int N;                // Number of nodes
    const char* instance_name;  // Instance name
    int M;                // Number of vehicles
    int Q;                // Capacity of a vehicle
    double** coords;
    int** jobs;
    double** D;           // Distance matrix
    double** T;           // Time matrix
    double* s;
    int* q;
    double* e;
    double* l;
} PDPTW;

PDPTW* create_problem(const int N, const char* instance_name);
void print_problem(PDPTW* problem);
void free_problem(PDPTW* problem);
PDPTW* read_pdptw_from_file(const char* filename);

#ifdef __cplusplus
}
#endif

#endif