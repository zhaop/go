#include <stdlib.h>
#include "utils.h"

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

int randi(int a, int b) {
	return rand() % (b - a) + a;
}
