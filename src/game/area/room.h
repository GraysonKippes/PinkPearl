#ifndef ROOM_H
#define ROOM_H

#include <stdint.h>

#include "game/entity/entity_spawner.h"
#include "math/Box.h"
#include "math/extent.h"
#include "math/offset.h"

#define NUM_ROOM_SIZES 4

extern const uint32_t num_room_sizes;

// The size of a room, given as ratio of the default room size (24x15).
typedef enum RoomSize {
	ROOM_SIZE_XS = 0,	// 8 x 5	-- Tiny room size, used in small bonus areas.
	ROOM_SIZE_S = 1,	// 16 x 10	-- Small room size, used in some dungeons.
	ROOM_SIZE_M = 2,	// 24 x 15	-- Default room size, used in the Overworld.
	ROOM_SIZE_L = 3		// 32 x 20	-- Large room size, used in some dungeons.
} RoomSize;

typedef struct Room {

	int id;
	RoomSize size;
	Extent extent;
	Offset position;
	
	// Contains the tile indices for each layer in this room.
	// First order represents a layer, second order represents tile indices for that layer.
	uint32_t **ppTileIndices;

	unsigned int wallCount;
	BoxD *pWalls;
	
	unsigned int num_entity_spawners;
	entity_spawner_t *entity_spawners;

} Room;

Extent room_size_to_extent(const RoomSize room_size);

void deleteRoom(Room room);

#endif	// ROOM_H
