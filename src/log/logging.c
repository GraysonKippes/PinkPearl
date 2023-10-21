#include "logging.h"

#include <stdarg.h>
#include <stdio.h>

#define LOG_OUT stderr

const char *enum_to_string(log_level_t level) {
	switch (level) {
		case VERBOSE: 	return "VERBOSE";
		case INFO: 	return "INFO";
		case WARNING:	return "WARNING";
		case ERROR:	return "ERROR";
		case FATAL:	return "FATAL";
	}
}

void log_message(log_level_t level, const char *message) {
	if (level < SEVERITY_THRESHOLD)
		return;
	fprintf(LOG_OUT, "[%s] %s\n", enum_to_string(level), message);
}

void logf_message(log_level_t level, const char *format, ...) {
	if (level < SEVERITY_THRESHOLD)
		return;
	fprintf(LOG_OUT, "[%s] ", enum_to_string(level));
	va_list args;
	va_start(args, format);
	vfprintf(LOG_OUT, format, args);
	va_end(args);
	fputc('\n', LOG_OUT);
}
