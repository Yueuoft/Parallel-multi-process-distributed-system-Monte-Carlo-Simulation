#include <stdlib.h>
#include <math.h>
#include "normal_rng.h"

double rand_uniform(void) {
    return (rand() + 1.0) / (RAND_MAX + 2.0);
}

double rand_normal(void) {
    static int has_spare = 0;
    static double spare = 0.0;

    if (has_spare) {
        has_spare = 0;
        return spare;
    }

    double u1 = rand_uniform();
    double u2 = rand_uniform();

    double radius = sqrt(-2.0 * log(u1));
    double theta = 2.0 * M_PI * u2;

    spare = radius * sin(theta);
    has_spare = 1;
    return radius * cos(theta);
}
