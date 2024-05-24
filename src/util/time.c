#include "time.h"

#include <sys/timeb.h>

unsigned long long int getTimeMS(void) {
	struct timeb time;
	ftime(&time);
	return (unsigned long long int)time.time * 1000LLU + (unsigned long long int)time.millitm;
}
