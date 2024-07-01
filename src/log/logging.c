#include "logging.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "log_stack.h"

#define LOG_OUT stderr

#define WIN32_FORMATTING

static const log_level_t severity_threshold = LOG_LEVEL_VERBOSE;

static const char *severity_labels[5] = {
	"LOG_LEVEL_VERBOSE",
	"LOG_LEVEL_INFO",
	"LOG_LEVEL_WARNING",
	"LOG_LEVEL_ERROR",
	"LOG_LEVEL_FATAL"
};

static const char *log_format(log_level_t level);
static const char *log_deformat(void);

// TODO - use safe "_s" io functions

void logMsg(log_level_t level, const char *message) {

	if (level < severity_threshold) {
		return;
	}

	pthread_mutex_lock(&log_stack_mutex);
	fprintf(LOG_OUT, "%s", log_format(level));
	fprintf(LOG_OUT, "[%s] (%s) %s", severity_labels[level], log_stack_get_string().buffer, message);
	fprintf(LOG_OUT, "%s\n", log_deformat());
	pthread_mutex_unlock(&log_stack_mutex);
}

void logMsgF(log_level_t level, const char *format, ...) {

	if (level < severity_threshold) {
		return;
	}

	pthread_mutex_lock(&log_stack_mutex);
	fprintf(LOG_OUT, "%s", log_format(level));
	fprintf(LOG_OUT, "[%s] (%s) ", severity_labels[level], log_stack_get_string().buffer);

	va_list args;
	va_start(args, format);
	vfprintf(LOG_OUT, format, args);
	va_end(args);

	fprintf(LOG_OUT, "%s\n", log_deformat());
	pthread_mutex_unlock(&log_stack_mutex);
}

#ifdef WIN32_FORMATTING
static const char *log_format(log_level_t level) {
	switch (level) {
		default:
		case LOG_LEVEL_VERBOSE: 	return "\x1B[40;90m";	// bg: Black	fg: Bright Black
		case LOG_LEVEL_INFO: 		return "\x1B[40;37m";	// bg: Black	fg: White
		case LOG_LEVEL_WARNING:	return "\x1B[40;93m";	// bg: Black	fg: Bright Yellow
		case LOG_LEVEL_ERROR:		return "\x1B[40;91m";	// bg: Black	fg: Bright Red
		case LOG_LEVEL_FATAL:		return "\x1B[41;97m";	// bg: Red		fg: Bright White
	}
}

static const char *log_deformat(void) {
	return "\x1B[0m";
}
#else
static const char *log_format(log_level_t level) {
	return "";
}

static const char *log_deformat(void) {
	return "";
}
#endif	// WIN32_FORMATTING
