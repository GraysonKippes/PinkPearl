#ifndef LOG_STACK_H
#define LOG_STACK_H

#include "util/string.h"

void init_log_stack(void);
void log_stack_push(const char *const pstring);
const string_t log_stack_get_string(void);
void log_stack_pop(void);
void terminate_log_stack(void);

#endif	// LOG_STACK_H