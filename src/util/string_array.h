#ifndef STRING_ARRAY_H
#define STRING_ARRAY_H

#include <stdbool.h>
#include <stddef.h>

typedef struct string_array_t {

	size_t num_strings;

	const char **strings;

} string_array_t;

bool is_string_array_empty(string_array_t string_array);

#endif	// STRING_ARRAY_H
