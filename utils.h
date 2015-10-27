#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

#define ALWAYS_INLINE __attribute__((always_inline))

long double timer_now();

int randi(int, int);
#define RANDI(a,b) (rand() % ((b) - (a)) + (a))

void seed_rand_once();

#endif