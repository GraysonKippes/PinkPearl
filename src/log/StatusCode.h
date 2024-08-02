#ifndef STATUS_CODE_H
#define STATUS_CODE_H

#include "log/logging.h"
#include "log/Logger.h"

typedef enum StatusCode {
	STATUS_CODE_GOOD = 0,
	STATUS_CODE_ALLOCATION_FAILED = -1,
	STATUS_CODE_FILE_READ_FAILED = -2,
	STATUS_CODE_MAX_OBJECTS_EXCEEDED = -3,
	STATUS_CODE_UNLOADING_UNUSED_RESOURCE = 1
} StatusCode;

#endif	// STATUS_CODE_H