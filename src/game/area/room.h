#ifndef ROOM_H
#define ROOM_H

#include "game/entity/entity_spawner.h"
#include "math/extent.h"
#include "math/hitbox.h"
#include "math/offset.h"

#include "tile.h"

#define NUM_ROOM_SIZES 4

extern const uint32_t num_room_sizes;

// The size of a room, given as ratio of the default room size (24x15).
typedef enum room_size_t {
	ONE_TO_THREE = 0,	// 8 x 5	-- Tiny room size, used in small bonus areas.
	TWO_TO_THREE = 1,	// 16 x 10	-- Small room size, used in some dungeons.
	THREE_TO_THREE = 2,	// 24 x 15	-- Default room size, used in the Overworld.
	FOUR_TO_THREE = 3	// 32 x 20	-- Large room size, used in some dungeons.
} room_size_t;

typedef struct room_t {

	int id;
	room_size_t size;
	extent_t extent;
	offset_t position;

	tile_t *tiles;

	unsigned int num_walls;
	rect_t *walls;
	
	unsigned int num_entity_spawners;
	entity_spawner_t *entity_spawners;

} room_t;

extent_t room_size_to_extent(const room_size_t room_size);

void destroy_room(room_t room);

#endif	// ROOM_H
