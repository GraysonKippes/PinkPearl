#ifndef EXTENT_H
#define EXTENT_H

#include <stdint.h>

typedef struct extent_t {
	uint32_t width;
	uint32_t length;
} extent_t;

uint64_t extent_area(const extent_t extent);

typedef struct offset_t {
	int32_t x;
	int32_t y;
} offset_t;

offset_t offset_add(const offset_t a, const offset_t b);

#endif	// EXTENT_H
