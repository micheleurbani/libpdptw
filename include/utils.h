#ifndef UTILS_H
#define UTILS_H
#include "problem.h"
#include "route.h"

int find_max(double *start, int count);
int find_min(double *start, int count);
double dist_shift(PDPTW* problem, int a, int b, int c);
double time_shift(PDPTW* problem, int a, int b, int c);

#endif