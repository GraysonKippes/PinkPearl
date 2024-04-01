#ifndef ERROR_CODE_H
#define ERROR_CODE_H

#include "log/logging.h"

typedef enum error_code_t {
	ERROR_CODE_ALLOCATION_FAILED,
	ERROR_CODE_FILE_READ_FAILED
} error_code_t;

char *error_code_str(const error_code_t error_code);

void error_queue_push(const log_level_t log_level, const error_code_t error_code);
void error_queue_flush(void);

#endif	// ERROR_CODE_H