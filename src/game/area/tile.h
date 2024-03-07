#ifndef TILE_H
#define TILE_H

#include <stdint.h>

typedef struct tile_t {
	uint32_t background_tilemap_slot;
	uint32_t foreground_tilemap_slot;
} tile_t;

#endif	// TILE_H
