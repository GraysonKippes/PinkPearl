#include "logger.h"

#include <stdarg.h>
#include <stdio.h>

const Logger loggerSystem = {
	.pName = "System",
	.levelThreshold = LOG_LEVEL_INFO
};

const Logger loggerGame = {
	.pName = "Game",
	.levelThreshold = LOG_LEVEL_INFO
};

const Logger loggerRender = {
	.pName = "Render",
	.levelThreshold = LOG_LEVEL_INFO
};

const Logger loggerVulkan = {
	.pName = "Vulkan",
	.levelThreshold = LOG_LEVEL_VERBOSE
};

const Logger loggerAudio = {
	.pName = "Audio",
	.levelThreshold = LOG_LEVEL_INFO
};

static const char *logFormat(const LogLevel level);

static const char *logDeformat(void);

static const char *logLevelLabels[5] = {
	"VERBOSE",
	"INFO",
	"WARNING",
	"ERROR",
	"FATAL"
};

void logMsg(const Logger logger, const LogLevel level, const char *const pMessage, ...) {
	if (level < logger.levelThreshold) {
		return;
	}
	
	fprintf(stderr, "%s", logFormat(level));
	fprintf(stderr, "[%s] (%s) ", logLevelLabels[level], logger.pName);

	va_list args;
	va_start(args, pMessage);
	vfprintf(stderr, pMessage, args);
	va_end(args);

	fprintf(stderr, "%s\n", logDeformat());
}

static const char *logFormat(const LogLevel level) {
	switch (level) {
		default:
		case LOG_LEVEL_VERBOSE: 	return "\x1B[40;90m";	// bg: Black	fg: Bright Black
		case LOG_LEVEL_INFO: 		return "\x1B[40;37m";	// bg: Black	fg: White
		case LOG_LEVEL_WARNING:		return "\x1B[40;93m";	// bg: Black	fg: Bright Yellow
		case LOG_LEVEL_ERROR:		return "\x1B[40;91m";	// bg: Black	fg: Bright Red
		case LOG_LEVEL_FATAL:		return "\x1B[41;97m";	// bg: Red		fg: Bright White
	}
}

static const char *logDeformat(void) {
	return "\x1B[0m";
}