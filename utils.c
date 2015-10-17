#include <stdlib.h>
#include "utils.h"

int randi(int a, int b) {
	return rand() % (b - a) + a;
}

