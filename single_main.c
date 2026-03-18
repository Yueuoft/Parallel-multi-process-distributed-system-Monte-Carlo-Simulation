#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "mc.h"
#include "bs.h"

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s S0 K r sigma T n_sims\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    MCParams params;
    params.S0 = atof(argv[1]);
    params.K = atof(argv[2]);
    params.r = atof(argv[3]);
    params.sigma = atof(argv[4]);
    params.T = atof(argv[5]);
    params.n_sims = atol(argv[6]);

    srand((unsigned int)time(NULL));

    Result res = run_monte_carlo_call(params);
    double mc_price = compute_discounted_price(&res, params.r, params.T);
    double se = compute_standard_error(&res, params.r, params.T);
    double bs_price = black_scholes_call(params.S0, params.K, params.r, params.sigma, params.T);

    printf("Single-process Monte Carlo\n");
    printf("MC price         = %.6f\n", mc_price);
    printf("Std error        = %.6f\n", se);
    printf("Black-Scholes    = %.6f\n", bs_price);

    return EXIT_SUCCESS;
}
