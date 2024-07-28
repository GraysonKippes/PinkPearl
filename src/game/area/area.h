#ifndef AREA_H
#define AREA_H

#include "math/extent.h"
#include "math/offset.h"
#include "math/vector3D.h"

#include "area_extent.h"
#include "room.h"

typedef struct Area {

	char *name;
	char *tilemap_name;

	// The extent of this area in number of rooms.
	area_extent_t extent;

	// The extent of each room in this area in tiles.
	RoomSize room_size;
	Extent room_extent;

	// The rooms in this area.
	uint32_t num_rooms;
	Room *pRooms;

	// Maps one-dimensional positions in the rectangular map to indices in the .pRooms array of rooms.
	// The length of this array should be equal to the area width times the area length.
	// Negative values in this array indicate that there is no room in the corresponding room space.
	int *pPositionsToRooms;

} Area;

bool areaGetRoom(const Area area, const Offset roomPosition, const Room **ppRoom);

int area_get_room_index(const Area area, const Offset room_position);

typedef enum CardinalDirection {
	DIRECTION_NONE = 0,
	DIRECTION_NORTH = 1,
	DIRECTION_EAST = 2,
	DIRECTION_SOUTH = 3,
	DIRECTION_WEST = 4
} CardinalDirection;

// Returns the direction in which the player is leaving the room, or NONE if the player is not leaving the room.
CardinalDirection test_room_travel(const Vector3D player_position, const Area area, const int current_room_index);

Offset direction_offset(const CardinalDirection direction);

#endif	// AREA_H
