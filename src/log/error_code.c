#include "error_code.h"

#include <stddef.h>

#include "util/allocate.h"

// TODO: support concurrency.
// Does not necessarily need to be atomic.

typedef struct error_queue_node_t {
	log_level_t log_level;
	error_code_t error_code;
	struct error_queue_node_t *next_node_ptr;
} error_queue_node_t;

typedef struct error_queue_t {
	error_queue_node_t *head_node_ptr;
	error_queue_node_t *tail_node_ptr;
} error_queue_t;

static error_queue_t error_queue;

void error_queue_push(const log_level_t log_level, const error_code_t error_code) {
	
	error_queue_node_t *new_node_ptr = NULL;
	allocate((void **)&new_node_ptr, 1, sizeof(error_queue_node_t));
	*new_node_ptr = (error_queue_node_t){
		.log_level = log_level,
		.error_code = error_code,
		.next_node_ptr = NULL
	};
	
	if (error_queue.head_node_ptr == NULL || error_queue.tail_node_ptr == NULL) {
		error_queue.head_node_ptr = new_node_ptr;
		error_queue.tail_node_ptr = new_node_ptr;
	}
	else if (error_queue.tail_node_ptr != NULL) {
		error_queue.tail_node_ptr->next_node_ptr = new_node_ptr;
		error_queue.tail_node_ptr = new_node_ptr;
	}
}

void error_queue_flush(void) {
	while (error_queue.head_node_ptr != NULL) {
		log_message(error_queue.head_node_ptr->log_level, error_code_str(error_queue.head_node_ptr->error_code));
		error_queue_node_t *previous_node_ptr = error_queue.head_node_ptr;
		error_queue.head_node_ptr = error_queue.head_node_ptr->next_node_ptr;
		deallocate((void **)&previous_node_ptr);
	}
	error_queue.tail_node_ptr = error_queue.head_node_ptr;
}

void terminate_error_queue(void) {
	while (error_queue.head_node_ptr != NULL) {
		error_queue_node_t *previous_node_ptr = error_queue.head_node_ptr;
		error_queue.head_node_ptr = error_queue.head_node_ptr->next_node_ptr;
		deallocate((void **)&previous_node_ptr);
	}
	error_queue.tail_node_ptr = error_queue.head_node_ptr;
}

char *error_code_str(const error_code_t error_code) {
	switch (error_code) {
		default: return "Unknown error.";
		case ERROR_CODE_ALLOCATION_FAILED: return "Allocation failed.";
		case ERROR_CODE_FILE_READ_FAILED: return "File read failed.";
		case ERROR_CODE_MAX_OBJECTS_EXCEEDED: return "Max objects to allocate exceeded by specified number of objects.";
	}
}