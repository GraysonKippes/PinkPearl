#include "logging.h"

#include <stdarg.h>
#include <stdio.h>

#define LOG_OUT stderr

#define OS_WINDOWS

const char *log_format(log_level_t level);

const char *log_deformat(log_level_t level);

const char *enum_to_string(log_level_t level) {
	switch (level) {
		case VERBOSE: 	return "VERBOSE";
		case INFO: 	return "INFO";
		case WARNING:	return "WARNING";
		case ERROR:	return "ERROR";
		case FATAL:	return "FATAL";
	}
	return "UNKNOWN";
}

void log_message(log_level_t level, const char *message) {
	if (level < SEVERITY_THRESHOLD)
		return;
	fprintf(LOG_OUT, "%s", log_format(level));
	fprintf(LOG_OUT, "[%s] %s\n", enum_to_string(level), message);
	fprintf(LOG_OUT, "%s", log_deformat(level));
}

void logf_message(log_level_t level, const char *format, ...) {
	if (level < SEVERITY_THRESHOLD)
		return;
	fprintf(LOG_OUT, "%s", log_format(level));
	fprintf(LOG_OUT, "[%s] ", enum_to_string(level));
	va_list args;
	va_start(args, format);
	vfprintf(LOG_OUT, format, args);
	va_end(args);
	fputc('\n', LOG_OUT);
	fprintf(LOG_OUT, "%s", log_deformat(level));
}

#ifdef OS_WINDOWS
const char *log_format(log_level_t level) {
	switch (level) {
		default:
		case VERBOSE: 	return "\x1B[37m";
		case INFO: 	return "\x1B[97m";
		case WARNING:	return "\x1B[93m";
		case ERROR:	return "\x1B[101m";
		case FATAL:	return "\x1B[41;97m";
	}
}

const char *log_deformat(log_level_t level) {
	return "\x1B[0m";
}
#else
const char *log_format(log_level_t level) {
	return "";
}

const char *log_deformat(log_level_t level) {
	return "";
}
#endif	// OS_WINDOWS
