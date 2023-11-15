#include "string_array.h"

bool is_string_array_empty(string_array_t string_array) {
	return string_array.num_strings == 0
		|| string_array.strings == NULL;
}
