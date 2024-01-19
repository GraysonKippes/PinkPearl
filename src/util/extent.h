#ifndef EXTENT_H
#define EXTENT_H

#include <stdint.h>

typedef struct extent_t {
	uint32_t width;
	uint32_t length;
} extent_t;

uint64_t extent_area(const extent_t extent);

#endif	// EXTENT_H
