#include "time.h"

#include <sys/timeb.h>

uint64_t get_time_ms(void) {

	struct timeb time;
	ftime(&time);

	return (uint64_t)time.time * 1000LL + (uint64_t)time.millitm;
}
