#ifndef EXTENT_H
#define EXTENT_H

#include <stdint.h>

typedef struct extent_t {
	uint32_t width;
	uint32_t length;
} extent_t;

// TODO - make this struct use signed integers.
typedef struct offset_t {
	uint32_t x;
	uint32_t y;
} offset_t;

#endif	// EXTENT_H
