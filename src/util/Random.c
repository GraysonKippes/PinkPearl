#include "Random.h"

#include <stdlib.h>
#include <time.h>

void initRandom(void) {
	srand(time(nullptr));
}

#define X(typename, functionName) typename functionName(const typename minimum, const typename maximum) { \
			return rand() % (maximum + 1 - minimum) + minimum; \
		}
LIST_OF_RANDOM_FUNCTIONS
#undef X