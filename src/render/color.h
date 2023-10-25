#ifndef COLOR_H
#define COLOR_H

#include <stdint.h>

typedef struct color3I_t {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
} color3I_t;

typedef struct color3F_t {
	float red;
	float green;
	float blue;
	float alpha;
} color3F_t;

#endif	// COLOR_H
