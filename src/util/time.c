#include "time.h"

#include <time.h>

unsigned long long int getTimeMS(void) {
	struct timespec ts = { };
	const int result = clock_gettime(CLOCK_REALTIME, &ts);
	if (!result) {
		return (unsigned long long int)ts.tv_sec * 1000LLU + (unsigned long long int)ts.tv_nsec / 1'000'000LLU;
	}
	return 0;
}
