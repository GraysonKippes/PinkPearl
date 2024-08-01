#ifndef LOGGER_H
#define LOGGER_H

typedef enum LogLevel {
	LOG_LEVEL_VERBOSE = 0,
	LOG_LEVEL_INFO = 1,
	LOG_LEVEL_WARNING = 2,
	LOG_LEVEL_ERROR = 3,
	LOG_LEVEL_FATAL = 4
} LogLevel;

typedef struct Logger {
	
	// The label that is prepended to each message generated with this logger.
	char *pName;
	
	// The lowest severity level of message that will be generated with this logger.
	LogLevel levelThreshold;
	
} Logger;

void logMsg(const Logger logger, const LogLevel level, const char *const pMessage, ...);

// Covers system-related tasks such as application start-up and clean-up as well as and file IO.
extern const Logger loggerSystem;

extern const Logger loggerGame;

extern const Logger loggerRender;

extern const Logger loggerVulkan;

extern const Logger loggerAudio;

#endif	// LOGGER_H