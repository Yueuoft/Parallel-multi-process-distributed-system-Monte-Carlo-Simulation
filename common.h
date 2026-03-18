#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define BACKLOG 10
#define BUF_SIZE 1024

typedef struct {
    double S0;
    double K;
    double r;
    double sigma;
    double T;
    long n_sims;
} MCParams;

typedef struct {
    double payoff_sum;
    double payoff_sq_sum;
    long num_sims;
} Result;

#endif
