#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stdbool.h>
#include <stddef.h>

typedef struct string_t {
	
	// Number of characters in the string.
	// Does not include the null-terminator.
	// Must be on range of [0, capacity).
	size_t length;
	
	// Total size of character buffer in bytes.
	size_t capacity;
	
	// Character buffer containing the string's contents.
	char *buffer;
	
} string_t;

string_t make_null_string(void);
string_t new_string_empty(const size_t capacity);
string_t new_string(const size_t capacity, const char *const initial_data);
bool destroy_string(string_t *const string_ptr);

// String analysis
bool is_string_null(const string_t string);
bool string_compare(const string_t a, const string_t b);
size_t string_reverse_search_char(const string_t string, const char c);

// String mutation
bool string_remove_trailing_chars(string_t *const string_ptr, const size_t num_chars);

// Miscellaneous
size_t string_hash(const string_t string, const size_t limit);

#endif	// UTIL_STRING_H