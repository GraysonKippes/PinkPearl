#include "string_array.h"

bool is_string_array_empty(string_array_t string_array) {
	return string_array.m_num_strings == 0
		|| string_array.m_strings == NULL;
}
