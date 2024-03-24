#include "string.h"

#include <stdlib.h>
#include <string.h>

#include "allocate.h"

string_t make_null_string(void) {
	return (string_t){
		.length = 0,
		.capacity = 0,
		.buffer = NULL
	};
}

string_t new_string_empty(const size_t capacity) {
	
	string_t string = make_null_string();
	if (!allocate((void **)&string.buffer, capacity, sizeof(char))) {
		return string;
	}
	string.capacity = capacity;
	
	return string;
}

string_t new_string(const size_t capacity, const char *const initial_data) {
	
	string_t string = new_string_empty(capacity);
	if (is_string_null(string)) {
		return string;
	}
	
	const size_t max_length = string.capacity - 1;
	// Length of initial data NOT including null terminator.
	const size_t initial_data_length = strlen(initial_data);
	strncpy_s(string.buffer, max_length, initial_data, initial_data_length);
	string.length = initial_data_length > max_length ? max_length : initial_data_length;
	
	return string;
}

bool destroy_string(string_t *const string_ptr) {
	
	if (string_ptr == NULL) {
		return false;
	}
	
	deallocate((void **)&string_ptr->buffer);
	string_ptr->length = 0;
	string_ptr->capacity = 0;
	
	return true;
}

bool string_compare(const string_t a, const string_t b) {
	
	if (is_string_null(a) || is_string_null(b)) {
		return false;
	}
	
	if (a.length != b.length) {
		return false;
	}
	
	return strncmp(a.buffer, b.buffer, a.length) == 0;
}

bool is_string_null(const string_t string) {
	return string.capacity == 0 || string.buffer == NULL;
}