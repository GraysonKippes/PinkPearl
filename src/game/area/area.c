#include "area.h"

#include <stddef.h>
#include <stdint.h>

#include "log/logging.h"

room_t *area_get_room_ptr(const area_t area, const offset_t map_position) {

	if (map_position.x < area.extent.x1 || map_position.x > area.extent.x2 
			|| map_position.y < area.extent.y1 || map_position.y > area.extent.y2) {
		logf_message(WARNING, "Warning getting room pointer: map position (%i, %i) of room falls outside of the area.", map_position.x, map_position.y);
		return NULL;
	}

	const int32_t area_width = area.extent.x2 - area.extent.x1;
	if (area_width < 1) {
		logf_message(WARNING, "Warning getting room pointer: the width of the area is not positive (%i).", area_width);
		return NULL;
	}

	const int32_t position_index = map_position.y * area_width + map_position.x;
	const int32_t room_index = area.positions_to_rooms[position_index];
	if (room_index >= area.num_rooms) {
		logf_message(WARNING, "Warning getting room pointer: room index (%i) is greater than or equal to number of rooms.", room_index);
		return NULL;
	}

	return area.rooms + room_index;
}

direction_t test_room_travel(const vector3D_cubic_t player_position, const area_t area, const int current_room_index) {
	
	if (current_room_index >= area.num_rooms) {
		logf_message(WARNING, "Warning testing room travel: specified current room index (%i) is not less than total number of rooms in specified area (%u).", current_room_index, area.num_rooms);
		return DIRECTION_ERROR;
	}

	const room_t room = area.rooms[current_room_index];
	const extent_t room_extent = area.room_extent;
	const vector3D_cubic_t room_position = {
		.x = (double)room.position.x * (double)room_extent.width,
		.y = (double)room.position.y * (double)room_extent.length,
		.z = 0.0
	};
	// Player position in room space (abbreviated IRS)
	const vector3D_cubic_t player_position_irs = vector3D_cubic_subtract(player_position, room_position);

	// TODO - make this check which edge of the room the player actually goes through.

	if (player_position_irs.x < -((double)room_extent.width / 2.0)) {
		return DIRECTION_WEST;
	}

	if (player_position_irs.x > ((double)room_extent.width / 2.0)) {
		return DIRECTION_EAST;
	}

	if (player_position_irs.y < -((double)room_extent.length / 2.0)) {
		return DIRECTION_SOUTH;
	}

	if (player_position_irs.y > ((double)room_extent.length / 2.0)) {
		return DIRECTION_NORTH;
	}

	return DIRECTION_NONE;
}
