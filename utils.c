#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "utils.h"

#ifdef __APPLE__

#include <mach/mach_time.h>

// Inspired by https://stackoverflow.com/a/5167506
static double orwl_timebase = 0.0;
static long double orwl_timestart = 0;

long double timer_now() {
  // be more careful in a multithreaded environement
  if (!orwl_timestart) {
    mach_timebase_info_data_t tb;
    mach_timebase_info(&tb);
    orwl_timebase = tb.numer;
    orwl_timebase /= tb.denom;
    orwl_timestart = (long double) mach_absolute_time();
  }
  return (long double) (mach_absolute_time() - orwl_timestart) * orwl_timebase;
}

#else

/* Available clocks:
CLOCK_REALTIME
CLOCK_REALTIME_COARSE
CLOCK_MONOTONIC
CLOCK_MONOTONIC_COARSE
CLOCK_MONOTONIC_RAW
CLOCK_BOOTTIME
CLOCK_PROCESS_CPUTIME_ID
CLOCK_THREAD_CPUTIME_ID
*/

long double timer_now() {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (long double) ts.tv_sec + 1e-9*(long double) ts.tv_nsec;
}
#endif

int randi(int a, int b) {
	return rand() % (b - a) + a;
}

void seed_rand_once() {
	// srand(time(NULL));
	srand(42);
}

int min(int a, int b) {
	return (a < b) ? a : b;
}

int max(int a, int b) {
	return (a > b) ? a : b;
}

// Pick a random item in arr that has given value
size_t pick_value_f(const float* arr, size_t n, float val, size_t occurrences) {
	size_t j = RANDI(0, occurrences);
	for (size_t i = 0; i < n; ++i) {
		if (arr[i] == val && j-- == 0) {
			return i;
		}
	}
	return -1;
}

// Pick a random item in arr that has given value
size_t pick_value_i(const int* arr, size_t n, int val, size_t occurrences) {
	size_t j = RANDI(0, occurrences);
	for (size_t i = 0; i < n; ++i) {
		if (arr[i] == val && j-- == 0) {
			return i;
		}
	}
	return -1;
}
