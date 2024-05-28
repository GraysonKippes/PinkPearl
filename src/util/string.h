#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stdbool.h>
#include <stddef.h>

typedef struct String {
	
	// Number of characters in the string.
	// Does not include the null-terminator.
	// Must be on range of [0, capacity).
	size_t length;
	
	// Total size of character buffer in bytes.
	size_t capacity;
	
	// Character buffer containing the string's contents.
	char *buffer;
	
} String;

String make_null_string(void);
String new_string_empty(const size_t capacity);
String new_string(const size_t capacity, const char *const initial_data);
bool destroy_string(String *const string_ptr);

// String analysis
bool is_string_null(const String string);
bool string_compare(const String a, const String b);
size_t string_reverse_search_char(const String string, const char c);

// String mutation
bool string_concatenate_char(String *const string_ptr, const char c);
bool string_concatenate_string(String *const dest_ptr, const String src);
bool string_concatenate_pstring(String *const dest_ptr, const char *const src_pstring);
bool string_remove_trailing_chars(String *const string_ptr, const size_t num_chars);

// Miscellaneous
size_t string_hash(const String string, const size_t limit);

#endif	// UTIL_STRING_H