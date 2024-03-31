#include "log_stack.h"

#include <stddef.h>

#include "config.h"
#include "log/logging.h"
#include "util/allocate.h"

typedef struct string_stack_node_t {
	string_t string;
	struct string_stack_node_t *next_node_ptr;
} string_stack_node_t;

typedef struct string_stack_t {
	string_stack_node_t *top_node_ptr;
	size_t height;
} string_stack_t;

static void string_stack_push(string_stack_t *const string_stack_ptr, const string_t string) {
	
	if (string_stack_ptr == NULL || is_string_null(string)) {
		return;
	}
	
	string_stack_node_t *new_node_ptr = NULL;
	allocate((void **)&new_node_ptr, 1, sizeof(string_stack_node_t));
	new_node_ptr->string = string;
	new_node_ptr->next_node_ptr = string_stack_ptr->top_node_ptr;
	string_stack_ptr->top_node_ptr = new_node_ptr;
	string_stack_ptr->height += 1;
}

static string_t string_stack_pop(string_stack_t *const string_stack_ptr) {
	
	if (string_stack_ptr == NULL) {
		return make_null_string();
	}
	
	if (string_stack_ptr->top_node_ptr == NULL || string_stack_ptr->height == 0) {
		return make_null_string();
	}
	
	const string_t string = string_stack_ptr->top_node_ptr->string;
	string_stack_node_t *temp_node_ptr = string_stack_ptr->top_node_ptr;
	string_stack_ptr->top_node_ptr = string_stack_ptr->top_node_ptr->next_node_ptr;
	deallocate((void **)&temp_node_ptr);
	string_stack_ptr->height -= 1;
	return string;
}

// Contains names of functions/procedures/engine layers.
static string_stack_t log_stack;

void init_log_stack(void) {
	log_stack = (string_stack_t){
		.top_node_ptr = NULL,
		.height = 0
	};
	string_stack_push(&log_stack, new_string(64, APP_NAME));
}

void log_stack_push(const string_t string) {
	string_stack_push(&log_stack, string);
}

string_t log_stack_peek(void) {
	if (log_stack.top_node_ptr == NULL) {
		return make_null_string();
	}
	return log_stack.top_node_ptr->string;
}

void log_stack_pop(void) {
	if (log_stack.height > 0) {
		// Discard pop result, not needed for log pop.
		string_stack_pop(&log_stack);
	}
}