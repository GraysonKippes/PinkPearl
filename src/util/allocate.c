#include "allocate.h"

#include <stdlib.h>

bool allocate(size_t num_objects, size_t num_bytes_per_object, void **ptr_ptr) {

	if (ptr_ptr != NULL && num_objects > 0 && num_bytes_per_object > 0) {
		void *ptr = calloc(num_objects, num_bytes_per_object);
		if (ptr != NULL) {
			*ptr_ptr = ptr;
			return true;
		}
	}

	return false;
}

bool deallocate(void **ptr_ptr) {

	if (ptr_ptr == NULL) {
		return false;
	}

	free(*ptr_ptr);
	*ptr_ptr = NULL;

	return true;
}