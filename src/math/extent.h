#ifndef EXTENT_H
#define EXTENT_H

#include <stdbool.h>
#include <stdint.h>

// TODO - consider switching to signed integers
typedef struct Extent {
	uint32_t width;
	uint32_t length;
} Extent;

// Returns true if both extents have the same width and length, false otherwise.
bool extent_equals(const Extent a, const Extent b);

Extent extent_scale(const Extent extent, const uint32_t factor);

uint64_t extentArea(const Extent extent);

#endif	// EXTENT_H
