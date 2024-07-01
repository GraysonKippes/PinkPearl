#ifndef LOGGING_H
#define LOGGING_H

typedef enum log_level_t {
	LOG_LEVEL_VERBOSE = 0,
	LOG_LEVEL_INFO = 1,
	LOG_LEVEL_WARNING = 2,
	LOG_LEVEL_ERROR = 3,
	LOG_LEVEL_FATAL = 4
} log_level_t;

void logMsg(log_level_t level, const char *message);
void logMsgF(log_level_t level, const char *format, ...);

#endif	// LOGGING_H
