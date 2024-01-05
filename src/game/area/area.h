#ifndef AREA_H
#define AREA_H

#include "util/extent.h"

#include "room.h"

typedef struct area_extent_t {
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
} area_extent_t;

typedef struct area_t {

	char *name;

	char *tilemap_name;

	// The extent of this area in number of rooms.
	area_extent_t extent;

	// The rooms in this area.
	uint32_t num_rooms;
	room_t *rooms;

	// Maps positions in the rectangular map to actual rooms in this area.
	// The 32-bit unsigned integer limit is used as a magic value to indicate that there is no room at the position;
	// 	this should not be an issue, as there should never be 4,294,967,295 rooms in an area.
	// 2D positions are converted to a 1D index into this array.
	// The length of this array is guaranteed to be equal to the area width times the area length.
	uint32_t *positions_to_rooms;

} area_t;

// Returns a pointer to the room at the map position in the area.
// Returns NULL if the room could not be found.
room_t *area_get_room_ptr(area_t area, offset_t map_position);

#endif	// AREA_H
