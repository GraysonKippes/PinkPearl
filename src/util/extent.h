#ifndef EXTENT_H
#define EXTENT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct extent_t {
	uint32_t width;
	uint32_t length;
} extent_t;

// Returns true if both extents have the same width and length, false otherwise.
bool extent_equals(const extent_t a, const extent_t b);

uint64_t extent_area(const extent_t extent);

#endif	// EXTENT_H
