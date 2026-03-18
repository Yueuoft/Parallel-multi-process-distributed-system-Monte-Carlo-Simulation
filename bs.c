#include <math.h>
#include "bs.h"

double normal_cdf(double x) {
    return 0.5 * (1.0 + erf(x / sqrt(2.0)));
}

double black_scholes_call(double S0, double K, double r, double sigma, double T) {
    double d1 = (log(S0 / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrt(T));
    double d2 = d1 - sigma * sqrt(T);

    return S0 * normal_cdf(d1) - K * exp(-r * T) * normal_cdf(d2);
}
