#ifndef ERROR_CODE_H
#define ERROR_CODE_H

#include "log/logging.h"
#include "log/Logger.h"

typedef enum ErrorCode {
	ERROR_CODE_ALLOCATION_FAILED,
	ERROR_CODE_FILE_READ_FAILED,
	ERROR_CODE_MAX_OBJECTS_EXCEEDED,
	ERROR_CODE_UNLOADING_UNUSED_RESOURCE
} ErrorCode;

char *error_code_str(const ErrorCode error_code);

void error_queue_push(const LogLevel logLevel, const ErrorCode errorCode);
void error_queue_flush(void);
void terminate_error_queue(void);

#endif	// ERROR_CODE_H