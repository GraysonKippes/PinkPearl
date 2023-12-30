#ifndef FILE_IO_H
#define FILE_IO_H

#include <stdio.h>

void read_data(FILE *restrict stream, const size_t num_bytes_per_object, const size_t num_objects, void *restrict buffer);

#endif	// FILE_IO_H
