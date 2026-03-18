#include <math.h>
#include "mc.h"
#include "normal_rng.h"

static double max_double(double a, double b) {
    return (a > b) ? a : b;
}

Result run_monte_carlo_call(MCParams params) {
    Result res;
    res.payoff_sum = 0.0;
    res.payoff_sq_sum = 0.0;
    res.num_sims = params.n_sims;

    double drift = (params.r - 0.5 * params.sigma * params.sigma) * params.T;
    double diffusion = params.sigma * sqrt(params.T);

    for (long i = 0; i < params.n_sims; i++) {
        double z = rand_normal();
        double ST = params.S0 * exp(drift + diffusion * z);
        double payoff = max_double(ST - params.K, 0.0);

        res.payoff_sum += payoff;
        res.payoff_sq_sum += payoff * payoff;
    }

    return res;
}

double compute_discounted_price(const Result *res, double r, double T) {
    if (res->num_sims <= 0) return 0.0;
    double mean_payoff = res->payoff_sum / res->num_sims;
    return exp(-r * T) * mean_payoff;
}

double compute_standard_error(const Result *res, double r, double T) {
    if (res->num_sims <= 1) return 0.0;

    double mean = res->payoff_sum / res->num_sims;
    double second = res->payoff_sq_sum / res->num_sims;
    double variance = second - mean * mean;
    if (variance < 0.0) variance = 0.0;

    return exp(-r * T) * sqrt(variance / res->num_sims);
}
