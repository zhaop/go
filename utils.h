#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdlib.h>

#define MANUAL_INLINE

long double timer_now();

int randi(int, int);
#define RANDI(a,b) (rand() % ((b) - (a)) + (a))

void seed_rand_once();

int min(int, int);
int max(int, int);

size_t pick_value_f(const float*, size_t, float, size_t);
size_t pick_value_i(const int*, size_t, int, size_t);

#endif