#ifndef ALLOCATE_H
#define ALLOCATE_H

#include <stddef.h>

typedef void *(*Allocator)(const size_t objectCount, const size_t objectSize);
typedef void *(*Reallocator)(void *const pMemory, const size_t objectCount, const size_t objectSize);
typedef void *(*Deallocator)(void *pMemory);

typedef struct AllocationFunctors {
	Allocator allocator;
	Reallocator reallocator;
	Deallocator deallocator;
} AllocationFunctors;

// Allocates at least objectCount * objectSize bytes on the heap.
// Returns nullptr if the allocation fails.
void *heapAlloc(const size_t objectCount, const size_t objectSize);

// Reallocates an existing memory object to at least objectCount * objectSize bytes on the heap.
// Returns the pointer to the original memory if the reallocation fails, which is still valid and must be freed.
void *heapRealloc(void *const pMemory, const size_t objectCount, const size_t objectSize);

// Reallocates an existing memory object to at least objectCount * objectSize bytes on the heap.
// Returns nullptr, assign the pointer variable being passed in as an argument to the return value of this function.
void *heapFree(void *pMemory);

#endif	// ALLOCATE_H
