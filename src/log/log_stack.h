#ifndef LOG_STACK_H
#define LOG_STACK_H

#include <pthread.h>

#include "util/string.h"

extern pthread_mutex_t log_stack_mutex;

void init_log_stack(void);
void log_stack_push(const char *const pstring);
string_t log_stack_get_string(void);
void log_stack_pop(void);
void terminate_log_stack(void);

#endif	// LOG_STACK_H