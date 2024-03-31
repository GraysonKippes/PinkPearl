#ifndef LOG_STACK_H
#define LOG_STACK_H

#include "util/string.h"

void init_log_stack(void);
void log_stack_push(const string_t string);
string_t log_stack_peek(void);
void log_stack_pop(void);

#endif	// LOG_STACK_H