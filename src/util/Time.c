#include "Time.h"

#include <errno.h>
#include <string.h>
#include <time.h>
#include "log/Logger.h"

uint64_t getMilliseconds(void) {
	struct timespec ts = { };
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		return (uint64_t)ts.tv_sec * 1000LLU + (uint64_t)ts.tv_nsec / 1'000'000LLU;
	}
	const errno_t error = errno;
	logMsg(loggerSystem, LOG_LEVEL_ERROR, "Getting milliseconds: error occurred getting clock time (error code = \"%s\").", strerror(error));
	return 0;
}
