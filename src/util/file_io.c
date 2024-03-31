#include "file_io.h"

#include <stddef.h>

bool read_data(FILE *restrict stream, const size_t num_bytes_per_object, const size_t num_objects, void *restrict buffer) {
	
	if (stream == NULL || buffer == NULL) {
		return false;
	}

	const size_t num_objects_read = fread(buffer, num_bytes_per_object, num_objects, stream);

	return num_objects_read == num_objects;

	/*
	if (num_objects_read < num_objects) {
		const int end_of_file = feof(stream);
		const int file_error = ferror(stream);
		return false;
	}
	*/
}

int read_string(FILE *restrict stream, const size_t max_str_len, char *str) {

	if (stream == NULL) {
		return -1;
	}

	if (str == NULL) {
		return -2;
	}

	char c = fgetc(stream);
	size_t i = 0;
	while (c != '\0' && i < max_str_len) {
		str[i++] = c;
		c = fgetc(stream);
	}

	// Return `1` as a warning if `max_str_len` is reached before the null-terminator is reached.
	if (i >= max_str_len) {
		return 1;
	}
	return 0;
}

string_t read_string2(FILE *restrict stream, const size_t max_string_length) {
	
	if (stream == NULL) {
		return make_null_string();
	}
	
	string_t string = new_string_empty(max_string_length);
	
	char c = fgetc(stream);
	size_t i = 0;
	while (c != '\0' && i < max_string_length) {
		string_concatenate_char(&string, c);
		c = fgetc(stream);
	}
	
	return string;
}
