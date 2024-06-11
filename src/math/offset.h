#ifndef OFFSET_H
#define OFFSET_H

#include <stdint.h>

typedef struct Offset {
	int32_t x;
	int32_t y;
} Offset;

Offset offset_add(const Offset a, const Offset b);

Offset offset_subtract(const Offset a, const Offset b);

#endif	// OFFSET_H
