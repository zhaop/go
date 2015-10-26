#ifndef UTILS_H
#define UTILS_H

#define ALWAYS_INLINE __attribute__((always_inline))

long double timer_now();

int randi(int, int);

void seed_rand_once();

#endif