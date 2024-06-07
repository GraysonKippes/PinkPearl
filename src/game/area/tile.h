#ifndef TILE_H
#define TILE_H

#include <stdint.h>

[[deprecated]]
typedef struct Tile {
	uint32_t background_tilemap_slot;
	uint32_t foreground_tilemap_slot;
} Tile;

#endif	// TILE_H
