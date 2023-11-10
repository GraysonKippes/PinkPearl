#ifndef STRING_ARRAY_H
#define STRING_ARRAY_H

#include <stddef.h>

typedef struct string_array_t {

	size_t m_num_strings;

	const char **m_strings;

} string_array_t;

#endif	// STRING_ARRAY_H
