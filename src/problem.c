#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "config.h"
#include "problem.h"


PDPTW* create_problem(int N, const char* instance_name) {
    PDPTW* problem = (PDPTW*)malloc(sizeof(PDPTW));
    problem->N = N;
    problem->instance_name = instance_name;
    problem->coords = (double**)malloc(N * sizeof(double*));
    for (size_t i = 0; i < N; i++)
    {
        problem->coords[i] = (double*)malloc(2 * sizeof(double));
    }
    problem->q = (int*)malloc(N * sizeof(int));
    problem->e = (double*)malloc(N * sizeof(double));
    problem->l = (double*)malloc(N * sizeof(double));
    problem->s = (double*)malloc(N * sizeof(double));
    problem->jobs = (int**)malloc(N / 2 * sizeof(int*));
    for (size_t i = 0; i < N / 2; i++)
    {
        problem->jobs[i] = (int*)malloc(2 * sizeof(int));
    }
    problem->D = (double**)malloc(N * sizeof(double*));
    for (size_t i = 0; i < N; i++)
    {
        problem->D[i] = (double*)malloc(N * sizeof(double*));
    }
    problem->T = problem->D;   // Time matrix
    return problem;
}

PDPTW* read_pdptw_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return NULL;
    }
    char line[MAX_STR_LEN];

    // Read the number of nodes
    int N = 0;
    while (fgets(line, MAX_STR_LEN, file)){++N;}; fclose(file);
    N = N - 1;

    PDPTW* problem = create_problem(N, filename);

    file = fopen(filename, "r");
    // Read the number of vehicles and the Q
    fgets(line, MAX_STR_LEN, file);
    int flag;
    int result = sscanf(line, "%d\t%d\t%d", &problem->M, &problem->Q, &flag);

    int i = 0;
    int j = 0;
    while (fgets(line, MAX_STR_LEN, file))
    {
        int id, p_id, d_id;
        sscanf(line, "%d\t%lf\t%lf\t%d\t%lf\t%lf\t%lf\t%d\t%d",
            &id,
            &problem->coords[i][0],
            &problem->coords[i][1],
            &problem->q[i],
            &problem->e[i],
            &problem->l[i],
            &problem->s[i],
            &p_id,
            &d_id
        );
        // Pickup and delivery
        if (p_id == 0 && d_id != 0) // A pickup node
        {
            if (j >= problem->N / 2) {
                fprintf(stderr, "Error: too many jobs for allocated array\n");
                exit(1);
            }
            problem->jobs[j][0] = id;
            problem->jobs[j][1] = d_id;
            j++;
        }
        i++;
    }
    // Compute the distance matrix
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            problem->D[i][j] = sqrt(
                pow(problem->coords[i][0] - problem->coords[j][0], 2) +
                pow(problem->coords[i][1] - problem->coords[j][1], 2)
            );
        }
    }
    // Remove infeasible arcs
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (problem->T[0][i] + problem->s[i] + problem->T[i][j] > problem->l[j]) {
                problem->D[i][j] = BIG_M;
            }
        }
    }

    fclose(file);

    return problem;
}

void print_problem(PDPTW* problem) {
    printf("Instance name: %s\n", problem->instance_name);
    printf("N. vehicles: %d\n", problem->M);
    printf("Capacity: %d\n", problem->Q);
    printf("N. nodes: %d\n", problem->N);
    // Print jobs in a compact way
    printf("Jobs: ");
    for (int i = 0; i < problem->N / 2; i++) {
        printf("(%d, %d) ", problem->jobs[i][0], problem->jobs[i][1]);
    }
    printf("\n");
}

void free_problem(PDPTW* problem) {
    for (size_t i = 0; i < problem->N; i++)
    {
        free(problem->coords[i]);
    }
    for (size_t i = 0; i < problem->N / 2; i++)
    {
        free(problem->jobs[i]);
    }
    for (size_t i = 0; i < problem->N; i++)
    {
        free(problem->D[i]);
    }
    free(problem->coords);
    free(problem->jobs);
    free(problem->D);
    free(problem->s);
    free(problem->q);
    free(problem->e);
    free(problem->l);
    if (problem != NULL) {
        free(problem);
    }
}