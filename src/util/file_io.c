#include "file_io.h"

#include <stddef.h>

bool read_data(FILE *restrict stream, const size_t num_bytes_per_object, const size_t num_objects, void *restrict buffer) {
	if (stream == nullptr || buffer == nullptr) {
		return false;
	}

	const size_t num_objects_read = fread(buffer, num_bytes_per_object, num_objects, stream);
	if (num_objects_read != num_objects) {
		//error_queue_push(LOG_LOG_LEVEL_ERROR, STATUS_CODE_FILE_READ_FAILED);
	}

	return num_objects_read == num_objects;
}

int read_string(FILE *restrict stream, const size_t max_str_len, char *str) {
	if (stream == nullptr) {
		return -1;
	}

	if (str == nullptr) {
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

String readString(FILE *restrict stream, const size_t max_string_capacity) {
	if (stream == nullptr) {
		return makeNullString();
	}
	
	const long int current_position = ftell(stream);
	if (current_position < 0) {
		//error_queue_push(LOG_LOG_LEVEL_ERROR, STATUS_CODE_FILE_READ_FAILED);
		return makeNullString();
	}
	
	// First pass: get total length of string in file.
	size_t string_length = 0;
	int ch = fgetc(stream);
	while (ch != '\0' && ch != EOF && string_length < max_string_capacity) {
		string_length += 1;
		ch = fgetc(stream);
	}
	size_t string_capacity = string_length + 1;
	
	// Seek back to initial position.
	const int seek_result = fseek(stream, current_position, SEEK_SET);
	if (seek_result != 0) {
		//error_queue_push(LOG_LOG_LEVEL_ERROR, STATUS_CODE_FILE_READ_FAILED);
		return makeNullString();
	}
	
	// Second pass: read characters from file into string buffer.
	String string = newStringEmpty(string_capacity);
	size_t counter = 0;
	ch = fgetc(stream);
	while (ch != '\0' && ch != EOF && counter < string_capacity) {
		string.pBuffer[counter++] = (char)ch;
		ch = fgetc(stream);
	}
	string.pBuffer[counter] = '\0';
	string.length = string_length;
	return string;
}

void file_next_block(FILE *restrict stream) {
	
	static const long int block_size = 32;
	
	const long int current_position = ftell(stream);
	if (current_position < 0) {
		//error_queue_push(LOG_LOG_LEVEL_ERROR, STATUS_CODE_FILE_READ_FAILED);
	}
	
	long int next_position = current_position;
	const long int remainder = next_position % block_size;
	if (remainder != 0) {
		next_position += block_size - remainder;
	}
	
	const int seek_result = fseek(stream, next_position, SEEK_SET);
	if (seek_result != 0) {
		//error_queue_push(LOG_LOG_LEVEL_ERROR, STATUS_CODE_FILE_READ_FAILED);
	}
}