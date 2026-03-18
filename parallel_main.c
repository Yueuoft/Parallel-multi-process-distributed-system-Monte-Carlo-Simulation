#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#include "common.h"
#include "mc.h"
#include "bs.h"

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s S0 K r sigma T n_sims n_workers\n", prog);
}

static void write_all(int fd, const void *buf, size_t count) {
    const char *p = buf;
    while (count > 0) {
        ssize_t n = write(fd, p, count);
        if (n < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        p += n;
        count -= (size_t)n;
    }
}

static void read_all(int fd, void *buf, size_t count) {
    char *p = buf;
    while (count > 0) {
        ssize_t n = read(fd, p, count);
        if (n < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (n == 0) {
            fprintf(stderr, "Unexpected EOF\n");
            exit(EXIT_FAILURE);
        }
        p += n;
        count -= (size_t)n;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 8) {
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

    int n_workers = atoi(argv[7]);
    if (n_workers <= 0 || params.n_sims <= 0) {
        fprintf(stderr, "n_sims and n_workers must be positive.\n");
        return EXIT_FAILURE;
    }

    int (*pipes)[2] = malloc(sizeof(int[2]) * n_workers);
    if (!pipes) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    long base = params.n_sims / n_workers;
    long rem = params.n_sims % n_workers;

    for (int i = 0; i < n_workers; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            free(pipes);
            return EXIT_FAILURE;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            free(pipes);
            return EXIT_FAILURE;
        }

        if (pid == 0) {
            close(pipes[i][0]);

            MCParams local = params;
            local.n_sims = base + (i < rem ? 1 : 0);

            srand((unsigned int)(time(NULL) ^ getpid() ^ (i << 16)));

            Result res = run_monte_carlo_call(local);
            write_all(pipes[i][1], &res, sizeof(res));
            close(pipes[i][1]);
            _exit(EXIT_SUCCESS);
        }

        close(pipes[i][1]);
    }

    Result total = {0.0, 0.0, 0};

    for (int i = 0; i < n_workers; i++) {
        Result partial;
        read_all(pipes[i][0], &partial, sizeof(partial));
        close(pipes[i][0]);

        total.payoff_sum += partial.payoff_sum;
        total.payoff_sq_sum += partial.payoff_sq_sum;
        total.num_sims += partial.num_sims;
    }

    for (int i = 0; i < n_workers; i++) {
        wait(NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec)
                   + (end.tv_nsec - start.tv_nsec) / 1e9;

    double mc_price = compute_discounted_price(&total, params.r, params.T);
    double se = compute_standard_error(&total, params.r, params.T);
    double bs_price = black_scholes_call(params.S0, params.K, params.r, params.sigma, params.T);

    printf("Parallel Monte Carlo with fork() + pipe()\n");
    printf("Workers          = %d\n", n_workers);
    printf("MC price         = %.6f\n", mc_price);
    printf("Std error        = %.6f\n", se);
    printf("Black-Scholes    = %.6f\n", bs_price);
    printf("Runtime          = %.6f sec\n", elapsed);

    free(pipes);
    return EXIT_SUCCESS;
}
