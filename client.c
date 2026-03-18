#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "common.h"

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 10) {
        fprintf(stderr, "Usage: %s host port S0 K r sigma T n_sims n_workers\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *host = argv[1];
    int port = atoi(argv[2]);

    double S0 = atof(argv[3]);
    double K = atof(argv[4]);
    double r = atof(argv[5]);
    double sigma = atof(argv[6]);
    double T = atof(argv[7]);
    long n_sims = atol(argv[8]);
    int n_workers = atoi(argv[9]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) die("socket");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        die("connect");
    }

    char req[BUF_SIZE];
    snprintf(
        req,
        sizeof(req),
        "PRICE %.10f %.10f %.10f %.10f %.10f %ld %d\n",
        S0, K, r, sigma, T, n_sims, n_workers
    );

    if (write(sockfd, req, strlen(req)) < 0) {
        die("write");
    }

    char resp[BUF_SIZE];
    ssize_t n = read(sockfd, resp, sizeof(resp) - 1);
    if (n < 0) die("read");
    resp[n] = '\0';

    printf("%s", resp);

    close(sockfd);
    return EXIT_SUCCESS;
}
