#ifndef AREA_H
#define AREA_H

#include "math/extent.h"
#include "math/offset.h"
#include "math/vector3D.h"

#include "area_extent.h"
#include "room.h"

typedef struct area_t {

	char *name;
	char *tilemap_name;

	// The extent of this area in number of rooms.
	area_extent_t extent;

	// The extent of each room in this area in tiles.
	RoomSize room_size;
	Extent room_extent;

	// The rooms in this area.
	uint32_t num_rooms;
	Room *rooms;

	// Maps positions in the rectangular map to actual rooms in this area.
	// The length of this array should be equal to the area width times the area length.
	int *positions_to_rooms;

} area_t;

bool area_get_room_ptr(const area_t area, const Offset room_position, const Room **room_pptr);

int area_get_room_index(const area_t area, const Offset room_position);

typedef enum direction_t {
	DIRECTION_ERROR = -1,
	DIRECTION_NONE = 0,
	DIRECTION_NORTH = 1,
	DIRECTION_EAST = 2,
	DIRECTION_SOUTH = 3,
	DIRECTION_WEST = 4
} direction_t;

// Returns the direction in which the player is leaving the room, or NONE if the player is not leaving the room.
direction_t test_room_travel(const Vector3D player_position, const area_t area, const int current_room_index);

Offset direction_offset(const direction_t direction);

#endif	// AREA_H
