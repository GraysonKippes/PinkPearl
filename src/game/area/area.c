#include "area.h"

#include <stddef.h>
#include <stdint.h>

#include "log/logging.h"

bool areaGetRoom(const Area area, const Offset roomPosition, const Room **ppRoom) {
	if (!ppRoom) {
		logMsg(LOG_LEVEL_ERROR, "Error getting room pointer: pointer to room pointer is null.");
		return false;
	}

	if (!area.pRooms) {
		logMsg(LOG_LEVEL_ERROR, "Error getting room pointer: area.rooms is null.");
		return false;
	}

	if (!area.pPositionsToRooms) {
		logMsg(LOG_LEVEL_ERROR, "Error getting room pointer: area.pPositionsToRooms is null.");
		return false;
	}

	// This is an index into the array that maps 1D positions to indices into the area room array.
	const int roomPositionIndex = area_extent_index(area.extent, roomPosition);
	if (roomPositionIndex < 0) {
		logMsgF(LOG_LEVEL_ERROR, "Error getting room pointer: room position index is negative (%i).", roomPositionIndex);
		return false;
	}

	const int roomIndex = area.pPositionsToRooms[roomPositionIndex];
	if (roomIndex < 0) {
		return false;
	}

	if (roomIndex >= (int)area.num_rooms) {
		logMsgF(LOG_LEVEL_ERROR, "Error getting room pointer: room index (%i) is not less than total number of rooms (%i).", roomIndex, (int)area.num_rooms);
		return false;
	}

	*ppRoom = &area.pRooms[roomIndex];
	if (roomPosition.x != (*ppRoom)->position.x || roomPosition.y != (*ppRoom)->position.y) {
		logMsgF(LOG_LEVEL_WARNING, "Warning getting room pointer: specified room position (%i, %i) does not match the gotten room's position (%i, %i).", 
				roomPosition.x, roomPosition.y, (*ppRoom)->position.x, (*ppRoom)->position.y);
	}
	return true;
}

int area_get_room_index(const Area area, const Offset roomPosition) {
	if (!area.pPositionsToRooms) {
		logMsg(LOG_LEVEL_ERROR, "Error getting room index: area.pPositionsToRooms is null.");
		return -2;
	}

	// This is an index into the array that maps 1D positions to indices into the area room array.
	const int roomPositionIndex = area_extent_index(area.extent, roomPosition);
	if (roomPositionIndex < 0) {
		logMsgF(LOG_LEVEL_ERROR, "Error getting room pointer: room position index is negative (%i).", roomPositionIndex);
		return -3;
	}

	return area.pPositionsToRooms[roomPositionIndex];
}

CardinalDirection test_room_travel(const Vector3D player_position, const Area area, const int current_room_index) {
	
	if (current_room_index >= (int)area.num_rooms) {
		logMsgF(LOG_LEVEL_ERROR, "Error testing room travel: specified current room index (%i) is not less than total number of rooms in specified area (%u).", current_room_index, area.num_rooms);
		return DIRECTION_NONE;
	}

	const Room room = area.pRooms[current_room_index];
	const Extent room_extent = area.room_extent;
	const Vector3D roomPosition = {
		.x = (double)room.position.x * (double)room_extent.width,
		.y = (double)room.position.y * (double)room_extent.length,
		.z = 0.0
	};
	// Player position in room space (abbreviated IRS).
	const Vector3D player_position_irs = vector3D_subtract(player_position, roomPosition);

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

Offset direction_offset(const CardinalDirection direction) {
	switch (direction) {
		case DIRECTION_NORTH:	return (Offset){ .x = 0, .y = 1 };
		case DIRECTION_EAST: 	return (Offset){ .x = 1, .y = 0 };
		case DIRECTION_SOUTH: 	return (Offset){ .x = 0, .y = -1 };
		case DIRECTION_WEST: 	return (Offset){ .x = -1, .y = 0 };
	}
	return (Offset){ .x = 0, .y = 0 };
}
