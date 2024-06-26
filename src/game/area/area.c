#include "area.h"

#include <stddef.h>
#include <stdint.h>

#include "log/logging.h"

bool area_get_room_ptr(const area_t area, const offset_t room_position, const room_t **room_pptr) {

	if (room_pptr == NULL) {
		log_message(ERROR, "Error getting room pointer: pointer to room pointer is NULL.");
		return false;
	}

	if (area.rooms == NULL) {
		log_message(ERROR, "Error getting room pointer: area.rooms is NULL.");
		return false;
	}

	if (area.positions_to_rooms == NULL) {
		log_message(ERROR, "Error getting room pointer: area.positions_to_rooms is NULL.");
		return false;
	}

	// This is an index into the array that maps 1D positions to indices into the area room array.
	const int room_position_index = area_extent_index(area.extent, room_position);
	if (room_position_index < 0) {
		logf_message(ERROR, "Error getting room pointer: room position index is negative (%i).", room_position_index);
		return false;
	}

	const int room_index = area.positions_to_rooms[room_position_index];

	if (room_index < 0) {
		return false;
	}

	if (room_index >= (int)area.num_rooms) {
		logf_message(ERROR, "Error getting room pointer: room index (%i) is not less than total number of rooms (%i).", room_index, (int)area.num_rooms);
		return false;
	}

	*room_pptr = area.rooms + room_index;

	if (room_position.x != (*room_pptr)->position.x || room_position.y != (*room_pptr)->position.y) {
		logf_message(WARNING, "Warning getting room pointer: specified room position (%i, %i) does not match the gotten room's position (%i, %i).", 
				room_position.x, room_position.y, (*room_pptr)->position.x, (*room_pptr)->position.y);
	}

	return true;
}

int area_get_room_index(const area_t area, const offset_t room_position) {

	if (area.positions_to_rooms == NULL) {
		log_message(ERROR, "Error getting room index: area.positions_to_rooms is NULL.");
		return -2;
	}

	// This is an index into the array that maps 1D positions to indices into the area room array.
	const int room_position_index = area_extent_index(area.extent, room_position);
	if (room_position_index < 0) {
		logf_message(ERROR, "Error getting room pointer: room position index is negative (%i).", room_position_index);
		return -3;
	}

	return area.positions_to_rooms[room_position_index];
}

direction_t test_room_travel(const vector3D_t player_position, const area_t area, const int current_room_index) {
	
	if (current_room_index >= (int)area.num_rooms) {
		logf_message(ERROR, "Error testing room travel: specified current room index (%i) is not less than total number of rooms in specified area (%u).", current_room_index, area.num_rooms);
		return DIRECTION_ERROR;
	}

	const room_t room = area.rooms[current_room_index];
	const extent_t room_extent = area.room_extent;
	const vector3D_t room_position = {
		.x = (double)room.position.x * (double)room_extent.width,
		.y = (double)room.position.y * (double)room_extent.length,
		.z = 0.0
	};
	// Player position in room space (abbreviated IRS).
	const vector3D_t player_position_irs = vector3D_subtract(player_position, room_position);

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

offset_t direction_offset(const direction_t direction) {
	
	switch (direction) {
		case DIRECTION_NORTH: return (offset_t){ .x = 0, .y = 1 };
		case DIRECTION_EAST: return (offset_t){ .x = 1, .y = 0 };
		case DIRECTION_SOUTH: return (offset_t){ .x = 0, .y = -1 };
		case DIRECTION_WEST: return (offset_t){ .x = -1, .y = 0 };
	}

	return (offset_t){ 0 };
}
