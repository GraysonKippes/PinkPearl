#ifndef ALLOCATE_H
#define ALLOCATE_H

#include <stdbool.h>
#include <stddef.h>

// Allocates (at least) `num_bytes_per_object` * `num_objects` bytes, which are zeroed-out.
// Returns true if the allocation was successful, false otherwise.
// If allocation fails, then the contents of ptr_ptr are not changed.
bool allocate(size_t num_objects, size_t num_bytes_per_object, void **ptr_ptr);

// Frees the memory stored at the pointer pointed to by `ptr_ptr`.
// Sets the pointer stored at `ptr_ptr` (`*ptr_ptr`) to NULL.
// Returns true normally, but returns false if `ptr_ptr` itself is NULL.
bool deallocate(void **ptr_ptr);

#endif	// ALLOCATE_H
