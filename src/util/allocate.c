#include "allocate.h"

#include <stdlib.h>

bool allocate(void **ppObject, const size_t num_objects, const size_t num_bytes_per_object) {

	// TODO - include automatic reallocation if ppObject is not nullptr.

	if (ppObject != nullptr && num_objects > 0 && num_bytes_per_object > 0) {
		void *ptr = calloc(num_objects, num_bytes_per_object);
		if (ptr != nullptr) {
			*ppObject = ptr;
			return true;
		}
	}

	//error_queue_push(LOG_LOG_LEVEL_ERROR, STATUS_CODE_ALLOCATION_FAILED);
	return false;
}

bool allocate_max(void **ppObject, const size_t num_objects, const size_t num_bytes_per_object, const size_t max_num_objects) {

	if (ppObject != nullptr && num_objects > 0 && num_bytes_per_object > 0) {
		size_t num_objects_to_allocate = num_objects;
		if (num_objects_to_allocate > max_num_objects) {
			num_objects_to_allocate = max_num_objects;
			//error_queue_push(LOG_LOG_LEVEL_WARNING, STATUS_CODE_MAX_OBJECTS_EXCEEDED);
		}
		void *ptr = calloc(num_objects, num_bytes_per_object);
		if (ptr != nullptr) {
			*ppObject = ptr;
			return true;
		}
	}

	//error_queue_push(LOG_LOG_LEVEL_ERROR, STATUS_CODE_ALLOCATION_FAILED);
	return false;
}

bool deallocate(void **const ppObject) {
	if (!ppObject) {
		return false;
	}

	if (*ppObject != nullptr) {
		free(*ppObject);
		*ppObject = nullptr;
	}

	return true;
}
