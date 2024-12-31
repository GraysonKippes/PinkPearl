#include "allocate.h"

#include <assert.h>
#include <stdlib.h>
#include "log/Logger.h"

void *heapAlloc(const size_t objectCount, const size_t objectSize) {
	if (objectCount == 0 || objectSize == 0) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Heap allocation: requested memory size is invalid (count = %llu, size = %llu).", objectCount, objectSize);
	}
	void *pMemory = calloc(objectCount, objectSize);
	if (!pMemory) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Heap allocation: failed to allocate %llu objects of %llu bytes each (%llu bytes total).", objectCount, objectSize, objectCount * objectSize);
	} else {
		logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Allocated %llu objects of %llu bytes each (%llu bytes total).", objectCount, objectSize, objectCount * objectSize);
	}
	return pMemory;
}

void *heapRealloc(void *const pMemory, const size_t objectCount, const size_t objectSize) {
	if (objectCount == 0 || objectSize == 0) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Heap reallocation: requested memory size is invalid (count = %llu, size = %llu).", objectCount, objectSize);
	}
	if (!pMemory) {
		logMsg(loggerSystem, LOG_LEVEL_WARNING, "Heap reallocation: pointer to memory to be reallocated is null.");
	}
	void *pNewMemory = realloc(pMemory, objectCount * objectSize);
	if (!pNewMemory) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Heap reallocation: failed to reallocate %llu objects of %llu bytes each (%llu bytes total).", objectCount, objectSize, objectCount * objectSize);
		return pMemory;
	} else {
		logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Reallocated %llu objects of %llu bytes each (%llu bytes total).", objectCount, objectSize, objectCount * objectSize);
	}
	return pNewMemory;
}

void *heapFree(void *pMemory) {
	if (!pMemory) {
		logMsg(loggerSystem, LOG_LEVEL_WARNING, "Heap deallocation: pointer to memory to be freed is null.");
	}
	free(pMemory);
	return nullptr;
}