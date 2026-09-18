#include "utils.h"

int find_min(double *start, int count) {
    int min = *start;
    for (int i = 1; i < count; i++) {
        if (*(start + i) < min) {
            min = *(start + i);
        }
    }
    return min;
}

int find_max(double *start, int count) {
    int max = *start;
    for (int i = 1; i < count; i++) {
        if (*(start + i) > max) {
            max = *(start + i);
        }
    }
    return max;
}

double dist_shift(PDPTW* problem, int a, int b, int c) {
    /* Compute the distance difference = d_ac + d_cb - d_ab. */
    return problem->D[a][c] + problem->D[c][a] - problem->D[a][b];
}

double time_shift(PDPTW* problem, int a, int b, int c) {
    /* Compute the time difference = d_ac + d_cb - d_ab. */
    return problem->T[a][c] + problem->T[c][a] + problem->s[c] - problem->T[a][b];
}