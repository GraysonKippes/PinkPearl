#include "allocate.h"

#include <stdlib.h>

bool allocate(void **ptr_ptr, const size_t num_objects, const size_t num_bytes_per_object) {

	// TODO - include automatic reallocation if ptr_ptr is not nullptr.

	if (ptr_ptr != nullptr && num_objects > 0 && num_bytes_per_object > 0) {
		void *ptr = calloc(num_objects, num_bytes_per_object);
		if (ptr != nullptr) {
			*ptr_ptr = ptr;
			return true;
		}
	}

	//error_queue_push(LOG_LOG_LEVEL_ERROR, STATUS_CODE_ALLOCATION_FAILED);
	return false;
}

bool allocate_max(void **ptr_ptr, const size_t num_objects, const size_t num_bytes_per_object, const size_t max_num_objects) {

	if (ptr_ptr != nullptr && num_objects > 0 && num_bytes_per_object > 0) {
		size_t num_objects_to_allocate = num_objects;
		if (num_objects_to_allocate > max_num_objects) {
			num_objects_to_allocate = max_num_objects;
			//error_queue_push(LOG_LOG_LEVEL_WARNING, STATUS_CODE_MAX_OBJECTS_EXCEEDED);
		}
		void *ptr = calloc(num_objects, num_bytes_per_object);
		if (ptr != nullptr) {
			*ptr_ptr = ptr;
			return true;
		}
	}

	//error_queue_push(LOG_LOG_LEVEL_ERROR, STATUS_CODE_ALLOCATION_FAILED);
	return false;
}

bool deallocate(void **ptr_ptr) {

	if (ptr_ptr == nullptr) {
		return false;
	}

	if (*ptr_ptr != nullptr) {
		free(*ptr_ptr);
		*ptr_ptr = nullptr;
	}

	return true;
}
