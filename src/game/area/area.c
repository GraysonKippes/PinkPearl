#include "area.h"

#include <stddef.h>
#include <stdint.h>

#include "log/Logger.h"

bool validateArea(const Area area) {
	return area.pRooms != nullptr
		&& area.currentRoomIndex >= 0
		&& area.currentRoomIndex < area.roomCount
		&& area.pPositionsToRooms != nullptr;
}

bool areaGetRoom(const Area area, const Offset roomPosition, Room **const ppRoom) {
	if (!validateArea(area)) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error getting room pointer: area struct found to be invalid.");
		return false;
	} else if (!ppRoom) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error getting room pointer: pointer to room pointer is null.");
		return false;
	}

	// This is an index into the array that maps 1D positions to indices into the area room array.
	const int roomPositionIndex = areaExtentIndex(area.extent, roomPosition);
	if (roomPositionIndex < 0) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error getting room pointer: room position index is negative (%i).", roomPositionIndex);
		return false;
	}

	const int roomIndex = area.pPositionsToRooms[roomPositionIndex];
	if (roomIndex < 0) {
		return false;
	}

	if (roomIndex >= area.roomCount) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error getting room pointer: room index (%i) is not less than total number of rooms (%i).", roomIndex, area.roomCount);
		return false;
	}

	*ppRoom = &area.pRooms[roomIndex];
	if (roomPosition.x != (*ppRoom)->position.x || roomPosition.y != (*ppRoom)->position.y) {
		logMsg(loggerGame, LOG_LEVEL_WARNING, "Warning getting room pointer: specified room position (%i, %i) does not match the gotten room's position (%i, %i).", 
				roomPosition.x, roomPosition.y, (*ppRoom)->position.x, (*ppRoom)->position.y);
	}
	return true;
}

bool areaGetCurrentRoom(const Area area, Room **const ppRoom) {
	if (!validateArea(area)) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error getting current room pointer: area struct found to be invalid.");
		return false;
	} else if (!ppRoom) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error getting current room pointer: pointer to room pointer is null.");
		return false;
	}
	
	*ppRoom = &area.pRooms[area.currentRoomIndex];
	
	return true;
}

int areaGetRoomIndex(const Area area, const Offset roomPosition) {
	if (!area.pPositionsToRooms) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error getting room index: area.pPositionsToRooms is null.");
		return -2;
	}

	// This is an index into the array that maps 1D positions to indices into the area room array.
	const int roomPositionIndex = areaExtentIndex(area.extent, roomPosition);
	if (roomPositionIndex < 0) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error getting room pointer: room position index is negative (%i).", roomPositionIndex);
		return -3;
	}

	return area.pPositionsToRooms[roomPositionIndex];
}

int areaExtentIndex(const BoxI areaExtent, const Offset roomPosition) {
	if (roomPosition.x < areaExtent.x1 || roomPosition.x > areaExtent.x2 || roomPosition.y < areaExtent.y1 || roomPosition.y > areaExtent.y2) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error calculating area extent index: roomPosition (%i, %i) does not fall within area extent [(%i, %i), (%i, %i)].",
			roomPosition.x, roomPosition.y, areaExtent.x1, areaExtent.y1, areaExtent.x2, areaExtent.y2);
		return -1;
	}

	const int offsetX = roomPosition.x - areaExtent.x1;
	const int offsetY = roomPosition.y - areaExtent.y1;

	return (offsetY * boxWidth(areaExtent)) + offsetX;
}

CardinalDirection test_room_travel(const Vector3D player_position, const Area area, const int current_room_index) {
	
	if (current_room_index >= area.roomCount) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error testing room travel: specified current room index (%i) is not less than total number of rooms in specified area (%i).", current_room_index, area.roomCount);
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
	const Vector3D player_position_irs = subVec(player_position, roomPosition);

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
