#ifndef ALLOCATE_H
#define ALLOCATE_H

#include <stdbool.h>
#include <stddef.h>

// Allocates (at least) `num_bytes_per_object` * `num_objects` bytes, which are zeroed-out.
// Returns true if the allocation was successful, false otherwise.
// If allocation fails, then the contents of ptr_ptr are not changed.
bool allocate(void **ptr_ptr, const size_t num_objects, const size_t num_bytes_per_object);
bool allocate_max(void **ptr_ptr, const size_t num_objects, const size_t num_bytes_per_object, const size_t max_num_objects);

// TODO - write reallocation function.

// Frees the memory stored at the pointer pointed to by `ptr_ptr`.
// Sets the pointer stored at `ptr_ptr` (`*ptr_ptr`) to NULL.
// Returns true normally, but returns false if `ptr_ptr` itself is NULL.
bool deallocate(void **ptr_ptr);

#endif	// ALLOCATE_H
