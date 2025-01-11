#include "Time.h"

#include <time.h>

uint64_t getTimeMS(void) {
	struct timespec ts = { };
	const int result = clock_gettime(CLOCK_REALTIME, &ts);
	if (!result) {
		return (uint64_t)ts.tv_sec * 1000LLU + (uint64_t)ts.tv_nsec / 1'000'000LLU;
	}
	return 0;
}
