#include "error_code.h"

#include <stddef.h>
#include "log/Logger.h"
#include "util/Allocation.h"

// TODO: support concurrency.
// Does not necessarily need to be lockless.

typedef struct error_queue_node_t {
	LogLevel logLevel;
	StatusCode errorCode;
	struct error_queue_node_t *pNextNode;
} error_queue_node_t;

typedef struct error_queue_t {
	error_queue_node_t *pHeadNode;
	error_queue_node_t *pTailNode;
} error_queue_t;

static error_queue_t error_queue;

void error_queue_push(const LogLevel logLevel, const StatusCode errorCode) {
	
	error_queue_node_t *new_node_ptr = heapAlloc(1, sizeof(error_queue_node_t));
	*new_node_ptr = (error_queue_node_t){
		.logLevel = logLevel,
		.errorCode = errorCode,
		.pNextNode = nullptr
	};
	
	if (error_queue.pHeadNode == nullptr || error_queue.pTailNode == nullptr) {
		error_queue.pHeadNode = new_node_ptr;
		error_queue.pTailNode = new_node_ptr;
	} else if (error_queue.pTailNode != nullptr) {
		error_queue.pTailNode->pNextNode = new_node_ptr;
		error_queue.pTailNode = new_node_ptr;
	}
}

void error_queue_flush(void) {
	while (error_queue.pHeadNode != nullptr) {
		logMsg(error_queue.pHeadNode->logLevel, error_code_str(error_queue.pHeadNode->errorCode));
		logMsg();
		error_queue_node_t *previous_node_ptr = error_queue.pHeadNode;
		error_queue.pHeadNode = error_queue.pHeadNode->pNextNode;
		previous_node_ptr = heapFree(previous_node_ptr);
	}
	error_queue.pTailNode = error_queue.pHeadNode;
}

void terminate_error_queue(void) {
	while (error_queue.pHeadNode != nullptr) {
		error_queue_node_t *previous_node_ptr = error_queue.pHeadNode;
		error_queue.pHeadNode = error_queue.pHeadNode->pNextNode;
		previous_node_ptr = heapFree(previous_node_ptr);
	}
	error_queue.pTailNode = error_queue.pHeadNode;
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