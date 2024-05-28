#ifndef FILE_IO_H
#define FILE_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "string.h"

bool read_data(FILE *restrict stream, const size_t num_bytes_per_object, const size_t num_objects, void *restrict buffer);

// Reads characters continuously from `stream`, putting them into `str`, 
// 	until either a null-terminator is found, or `max_str_len` is reached.
// Returns 0 if the operation was successful, or 1 is `max_str_len` was reached;
// returns -1 if `stream` is NULL, or -2 if `str` is NULL.
int read_string(FILE *restrict stream, const size_t max_str_len, char *str);

String read_string2(FILE *restrict stream, const size_t max_string_capacity);

void file_next_block(FILE *restrict stream);

#endif	// FILE_IO_H
