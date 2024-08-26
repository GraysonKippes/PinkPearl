#ifndef STRING_ARRAY_H
#define STRING_ARRAY_H

#include <stdbool.h>
#include <stddef.h>

typedef struct StringArray {

	size_t num_strings;

	const char **strings;

} StringArray;

bool is_string_array_empty(StringArray string_array);

#endif	// STRING_ARRAY_H
