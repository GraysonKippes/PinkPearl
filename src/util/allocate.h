#ifndef ALLOCATE_H
#define ALLOCATE_H

#include <stdbool.h>
#include <stddef.h>

// Allocates (at least) `num_bytes_per_object` * `num_objects` bytes, which are zeroed-out.
// Returns true if the allocation was successful, false otherwise.
// If allocation fails, then the contents of ppObject are not changed.
bool allocate(void **ppObject, const size_t num_objects, const size_t num_bytes_per_object);
bool allocate_max(void **ppObject, const size_t num_objects, const size_t num_bytes_per_object, const size_t max_num_objects);

// TODO - write reallocation function.

// Frees the memory stored at the pointer pointed to by `ppObject`.
// Sets the pointer stored at `ppObject` (`*ppObject`) to nullptr.
// Returns true normally, but returns false if `ppObject` itself is nullptr.
bool deallocate(void **const ppObject);

typedef void *(*Allocator)(const size_t size);

typedef void *(*Reallocator)(void *const pBuffer, const size_t size);

typedef void (*Deallocator)(void *const pBuffer);

typedef struct AllocationFunctors {
	Allocator allocator;
	Reallocator reallocator;
	Deallocator deallocator;
} AllocationFunctors;

#endif	// ALLOCATE_H
