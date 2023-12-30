#include "file_io.h"

void read_data(FILE *restrict stream, const size_t num_bytes_per_object, const size_t num_objects, void *restrict buffer) {
	
	const size_t num_objects_read = fread(buffer, num_bytes_per_object, num_objects, stream);

	if (num_objects_read < num_objects) {
		const int end_of_file = feof(stream);
		const int file_error = ferror(stream);
	}
}
