#ifndef OFFSET_H
#define OFFSET_H

#include <stdint.h>

typedef struct offset_t {
	int32_t x;
	int32_t y;
} offset_t;

offset_t offset_add(const offset_t a, const offset_t b);

offset_t offset_subtract(const offset_t a, const offset_t b);

#endif	// OFFSET_H
