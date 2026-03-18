#ifndef MC_H
#define MC_H

#include "common.h"

Result run_monte_carlo_call(MCParams params);
double compute_discounted_price(const Result *res, double r, double T);
double compute_standard_error(const Result *res, double r, double T);

#endif
