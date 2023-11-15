#ifndef ROOM_H
#define ROOM_H

#include <stdio.h>

#include "util/extent.h"

#include "tile.h"

// The size of a room, given as ratio of the default room size (24x15).
typedef enum room_size_t {
	ONE_TO_ONE = 0,		// 24 x 15	-- Default room size, used in the Overworld.
	ONE_TO_THREE = 1,	// 8 x 5	-- Tiny room size, used in small bonus areas.
	TWO_TO_THREE = 2,	// 16 x 10	-- Small room size, used in some dungeons.
	FOUR_TO_THREE = 3	// 32 x 20	-- Large room size, used in some dungeons.
} room_size_t;

typedef struct room_t {

	extent_t extent;
	tile_t *tiles;

} room_t;

int read_room_data(FILE *file, room_t *room_ptr);

void destroy_room(room_t room);

#endif	// ROOM_H
