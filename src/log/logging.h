#ifndef LOGGING_H
#define LOGGING_H

typedef enum log_level_t {
	VERBOSE = 0,
	INFO = 1,
	WARNING = 2,
	ERROR = 3,
	FATAL = 4
} log_level_t;

void log_message(log_level_t level, const char *message);

void logf_message(log_level_t level, const char *format, ...);

#endif	// LOGGING_H
