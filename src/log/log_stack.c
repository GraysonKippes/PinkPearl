#include "log_stack.h"

#include <stddef.h>

#include "config.h"
#include "log/logging.h"
#include "util/allocate.h"

static const char log_stack_delimiter = '/';

typedef struct log_stack_t {
	string_t string;
	size_t height;	// Number of sub-strings in stack.
} log_stack_t;

// Contains names of functions/procedures/engine layers.
static log_stack_t log_stack;
pthread_mutex_t log_stack_mutex;

void init_log_stack(void) {
	pthread_mutex_init(&log_stack_mutex, NULL);
	log_stack = (log_stack_t){
		.string = new_string_empty(64),
		.height = 0
	};
	log_stack_push("PinkPearl");
}

void log_stack_push(const char *const pstring) {
	pthread_mutex_lock(&log_stack_mutex);
	if (log_stack.height > 0) {
		string_concatenate_char(&log_stack.string, log_stack_delimiter);
	}
	string_concatenate_pstring(&log_stack.string, pstring);
	log_stack.height += 1;
	pthread_mutex_unlock(&log_stack_mutex);
}

const string_t log_stack_get_string(void) {
	return log_stack.string;
}

void log_stack_pop(void) {
	pthread_mutex_lock(&log_stack_mutex);
	if (log_stack.height > 0) {
		const size_t delimiter_position = string_reverse_search_char(log_stack.string, log_stack_delimiter);
		string_remove_trailing_chars(&log_stack.string, log_stack.string.length - delimiter_position);
		log_stack.height -= 1;
	}
	pthread_mutex_unlock(&log_stack_mutex);
}

void terminate_log_stack(void) {
	destroy_string(&log_stack.string);
	log_stack.height = 0;
	pthread_mutex_destroy(&log_stack_mutex);
}