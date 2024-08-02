#include "error_code.h"

#include <stddef.h>

#include "util/allocate.h"

// TODO: support concurrency.
// Does not necessarily need to be atomic.

typedef struct error_queue_node_t {
	LogLevel logLevel;
	StatusCode errorCode;
	struct error_queue_node_t *next_node_ptr;
} error_queue_node_t;

typedef struct error_queue_t {
	error_queue_node_t *head_node_ptr;
	error_queue_node_t *tail_node_ptr;
} error_queue_t;

static error_queue_t error_queue;

void error_queue_push(const LogLevel logLevel, const StatusCode errorCode) {
	
	error_queue_node_t *new_node_ptr = nullptr;
	allocate((void **)&new_node_ptr, 1, sizeof(error_queue_node_t));
	*new_node_ptr = (error_queue_node_t){
		.logLevel = logLevel,
		.errorCode = errorCode,
		.next_node_ptr = nullptr
	};
	
	if (error_queue.head_node_ptr == nullptr || error_queue.tail_node_ptr == nullptr) {
		error_queue.head_node_ptr = new_node_ptr;
		error_queue.tail_node_ptr = new_node_ptr;
	}
	else if (error_queue.tail_node_ptr != nullptr) {
		error_queue.tail_node_ptr->next_node_ptr = new_node_ptr;
		error_queue.tail_node_ptr = new_node_ptr;
	}
}

void error_queue_flush(void) {
	while (error_queue.head_node_ptr != nullptr) {
		logMsg(error_queue.head_node_ptr->logLevel, error_code_str(error_queue.head_node_ptr->errorCode));
		logMsg();
		error_queue_node_t *previous_node_ptr = error_queue.head_node_ptr;
		error_queue.head_node_ptr = error_queue.head_node_ptr->next_node_ptr;
		deallocate((void **)&previous_node_ptr);
	}
	error_queue.tail_node_ptr = error_queue.head_node_ptr;
}

void terminate_error_queue(void) {
	while (error_queue.head_node_ptr != nullptr) {
		error_queue_node_t *previous_node_ptr = error_queue.head_node_ptr;
		error_queue.head_node_ptr = error_queue.head_node_ptr->next_node_ptr;
		deallocate((void **)&previous_node_ptr);
	}
	error_queue.tail_node_ptr = error_queue.head_node_ptr;
}

char *error_code_str(const StatusCode errorCode) {
	switch (errorCode) {
		default: return "Unknown error.";
		case STATUS_CODE_ALLOCATION_FAILED: return "Allocation failed.";
		case STATUS_CODE_FILE_READ_FAILED: return "File read failed.";
		case STATUS_CODE_MAX_OBJECTS_EXCEEDED: return "Max objects to allocate exceeded by specified number of objects.";
		case STATUS_CODE_UNLOADING_UNUSED_RESOURCE: return "Attempting to unload an unused/unallocated resource.";
	}
}