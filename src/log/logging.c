#include "logging.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#define LOG_OUT stderr

#define OS_WINDOWS

static const log_level_t severity_threshold = VERBOSE;

static const char *log_format(log_level_t level);

static const char *log_deformat(log_level_t level);

static const char *severity_labels[5] = {
	"VERBOSE",
	"INFO",
	"WARNING",
	"ERROR",
	"FATAL"
};

// TODO - use safe "_s" io functions

void log_message(log_level_t level, const char *message) {

	if (level < severity_threshold) {
		return;
	}

	fprintf(LOG_OUT, "%s", log_format(level));
	fprintf(LOG_OUT, "[%s] %s", severity_labels[level], message);
	fprintf(LOG_OUT, "%s\n", log_deformat(level));
}

void logf_message(log_level_t level, const char *format, ...) {

	if (level < severity_threshold) {
		return;
	}

	fprintf(LOG_OUT, "%s", log_format(level));
	fprintf(LOG_OUT, "[%s] ", severity_labels[level]);

	va_list args;
	va_start(args, format);
	vfprintf(LOG_OUT, format, args);
	va_end(args);

	fprintf(LOG_OUT, "%s\n", log_deformat(level));
}

#ifdef OS_WINDOWS
static const char *log_format(log_level_t level) {
	switch (level) {
		default:
		case VERBOSE: 	return "\x1B[40;90m";	// bg: Black	fg: Bright Black
		case INFO: 	return "\x1B[40;37m";	// bg: Black	fg: White
		case WARNING:	return "\x1B[40;93m";	// bg: Black	fg: Bright Yellow
		case ERROR:	return "\x1B[40;91m";	// bg: Black	fg: Bright Red
		case FATAL:	return "\x1B[41;97m";	// bg: Red	fg: Bright White
	}
}

static const char *log_deformat(log_level_t level) {
	return "\x1B[0m";
}
#else
static const char *log_format(log_level_t level) {
	return "";
}

static const char *log_deformat(log_level_t level) {
	return "";
}
#endif	// OS_WINDOWS
