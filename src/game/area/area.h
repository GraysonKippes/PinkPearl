#ifndef AREA_H
#define AREA_H

#include "math/Box.h"
#include "math/extent.h"
#include "math/offset.h"
#include "math/Vector.h"
#include "render/render_config.h"
#include "render/vulkan/TextureState.h"
#include "render/vulkan/math/projection.h"
#include "room.h"

typedef struct AreaRenderState {
	
	int32_t renderObjectHandle;
	int32_t currentRoomQuadIndices[2];
	int32_t nextRoomQuadIndices[2];
	
	int32_t wireframesHandle;
	int32_t currentRoomWireframes[16];
	int32_t nextRoomWireframes[16];
	
	TextureState tilemapTextureState;
	
	// Indexes into the array of room render object handles.
	// When the camera is not scrolling, current cache slot equals next cache slot.
	uint32_t currentCacheSlot;
	uint32_t nextCacheSlot;
	uint64_t scrollStartTimeMS;
	
	// Maps room IDs to room texture cache slots.
	uint32_t numRoomIDs;			// Equal to number of rooms.
	uint32_t *roomIDsToCacheSlots;	// UINT32_MAX represents unmapped room ID.
	Offset *roomIDsToPositions;
	
	// Maps room texture cache slots to room positions.
	// UINT32_MAX represents unmapped cache slot.
	uint32_t cacheSlotsToRoomIDs[2];
	
} AreaRenderState;

typedef struct Area {

	char *pName;
	char *pTilemapName;

	// The extent of this area in number of rooms.
	BoxI extent;

	// The extent of each room in this area in tiles.
	RoomSize room_size;
	Extent room_extent;

	// Information regarding rooms in this area.
	int32_t roomCount;			// Total number of rooms in this area.
	int32_t currentRoomIndex;	// The index of the room that the player is currently in.
	Room *pRooms;			// Array of all rooms in this area.

	// Maps one-dimensional positions in the rectangular map to indices in the .pRooms array of rooms.
	// The length of this array should be equal to the area width times the area length.
	// Negative values in this array indicate that there is no room in the corresponding room space.
	int32_t *pPositionsToRooms;
	
	AreaRenderState renderState;

} Area;

typedef enum CardinalDirection {
	DIRECTION_NONE = 0,
	DIRECTION_NORTH = 1,
	DIRECTION_EAST = 2,
	DIRECTION_SOUTH = 3,
	DIRECTION_WEST = 4
} CardinalDirection;

bool validateArea(const Area area);

// Gives the pointer to the room at the position in the area.
// Returns true if the retrieval was successful, false otherwise.
bool areaGetRoom(const Area area, const Offset roomPosition, Room **const ppRoom);

// Gives the pointer to the current room in the area.
// Returns true if the retrieval was successful, false otherwise.
bool areaGetCurrentRoom(const Area area, Room **const ppRoom);

// Sets the next room to the room in the given direction from the current room, if it exists.
// Returns true if the room exists and scrolling begins, false otherwise.
bool areaSetNextRoom(Area *const pArea, const CardinalDirection direction);

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

// Returns the direction in which the player is leaving the room, or NONE if the player is not leaving the room.
CardinalDirection test_room_travel(const Vector3D player_position, const Area area, const int current_room_index);

Offset direction_offset(const CardinalDirection direction);

// Resets the render state of a new created area.
// Call this directly after generating a new area.
void areaRenderStateReset(Area *const pArea, const Room initialRoom);

// Returns true if the current cache slot and the next cache slot are not equal, false otherwise.
bool areaIsScrolling(const Area area);

Vector4F areaGetCameraPosition(Area *const pArea);

ProjectionBounds areaGetProjectionBounds(const Area area);

void areaLoadWireframes(Area *const pArea);

void areaUnloadWireframes(Area *const pArea);

#endif	// AREA_H
