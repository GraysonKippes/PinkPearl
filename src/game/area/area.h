#ifndef AREA_H
#define AREA_H

#include "math/Box.h"
#include "math/extent.h"
#include "math/offset.h"
#include "math/Vector.h"

#include "room.h"

typedef struct Area {

	char *pName;
	char *pTilemapName;

	// The extent of this area in number of rooms.
	BoxI extent;

	// The extent of each room in this area in tiles.
	RoomSize room_size;
	Extent room_extent;

	// Information regarding rooms in this area.
	int roomCount;			// Total number of rooms in this area.
	int currentRoomIndex;	// The index of the room that the player is currently in.
	Room *pRooms;			// Array of all rooms in this area.

	// Maps one-dimensional positions in the rectangular map to indices in the .pRooms array of rooms.
	// The length of this array should be equal to the area width times the area length.
	// Negative values in this array indicate that there is no room in the corresponding room space.
	int *pPositionsToRooms;

} Area;

bool validateArea(const Area area);

// Gives the pointer to the room at the position in the area.
// Returns true if the retrieval was successful, false otherwise.
bool areaGetRoom(const Area area, const Offset roomPosition, Room **const ppRoom);

// Gives the pointer to the current room in the area.
// Returns true if the retrieval was successful, false otherwise.
bool areaGetCurrentRoom(const Area area, Room **const ppRoom);

// Returns the index of the room at the position in the area. 
int areaGetRoomIndex(const Area area, const Offset roomPosition);

// Returns a one-dimension index into the `area_extent` that corresponds to `position`.
// Because room position in an area is in the category of game-space coordinates, 
// 	they use the bottom-left to top-right orientation; in other words,
// 	the smallest coordinates represent the bottom-left corner of the area, and
// 	the largest coordinates represent the top-right corner of the area.
// If `position` does not correspond to any position in `area_extent`,
// 	then this function returns a negative value.
int areaExtentIndex(const BoxI areaExtent, const Offset roomPosition);

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
