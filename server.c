#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include "common.h"
#include "mc.h"
#include "bs.h"

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void write_all(int fd, const void *buf, size_t count) {
    const char *p = buf;
    while (count > 0) {
        ssize_t n = write(fd, p, count);
        if (n < 0) die("write");
        p += n;
        count -= (size_t)n;
    }
}

static void read_all(int fd, void *buf, size_t count) {
    char *p = buf;
    while (count > 0) {
        ssize_t n = read(fd, p, count);
        if (n < 0) die("read");
        if (n == 0) {
            fprintf(stderr, "Unexpected EOF\n");
            exit(EXIT_FAILURE);
        }
        p += n;
        count -= (size_t)n;
    }
}

static Result run_parallel_local(MCParams params, int n_workers) {
    int (*pipes)[2] = malloc(sizeof(int[2]) * n_workers);
    if (!pipes) die("malloc");

    long base = params.n_sims / n_workers;
    long rem = params.n_sims % n_workers;

    for (int i = 0; i < n_workers; i++) {
        if (pipe(pipes[i]) < 0) die("pipe");

        pid_t pid = fork();
        if (pid < 0) die("fork");

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

    free(pipes);
    return total;
}

static void handle_client(int client_fd) {
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        perror("read client");
        close(client_fd);
        return;
    }

    MCParams params;
    int n_workers;

    int parsed = sscanf(
        buf,
        "PRICE %lf %lf %lf %lf %lf %ld %d",
        &params.S0,
        &params.K,
        &params.r,
        &params.sigma,
        &params.T,
        &params.n_sims,
        &n_workers
    );

    if (parsed != 7 || n_workers <= 0 || params.n_sims <= 0) {
        const char *err = "ERROR invalid request\n";
        write_all(client_fd, err, strlen(err));
        close(client_fd);
        return;
    }

    Result total = run_parallel_local(params, n_workers);
    double price = compute_discounted_price(&total, params.r, params.T);
    double se = compute_standard_error(&total, params.r, params.T);
    double bs = black_scholes_call(params.S0, params.K, params.r, params.sigma, params.T);

    char out[BUF_SIZE];
    snprintf(out, sizeof(out), "RESULT %.6f %.6f %.6f\n", price, se, bs);
    write_all(client_fd, out, strlen(out));
    close(client_fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) die("socket");

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        die("setsockopt");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("bind");
    if (listen(server_fd, BACKLOG) < 0) die("listen");

    printf("Server listening on port %d\n", port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        handle_client(client_fd);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
