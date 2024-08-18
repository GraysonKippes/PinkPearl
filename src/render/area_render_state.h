#ifndef AREA_RENDER_STATE_H
#define AREA_RENDER_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "game/area/area.h"
#include "game/area/room.h"
#include "math/Box.h"
#include "math/offset.h"
#include "vulkan/TextureState.h"
#include "vulkan/math/projection.h"
#include "vulkan/math/vector4F.h"

#include "render_config.h"

typedef struct RoomRenderState {
	
	int roomID;
	
	// Contains the index for each quad of this room.
	// Each room layer has one quad.
	int quadIndices[NUM_ROOM_LAYERS];
	
} RoomRenderState;

typedef struct AreaRenderState {
	
	BoxI areaExtent;
	
	RoomSize roomSize;
	
	// The render object handle for each room in the room cache.
	// TODO - switch to using a single render object for the entire area, with quads for each room/layer.
	[[deprecated]]
	int roomRenderObjHandles[NUM_ROOM_TEXTURE_CACHE_SLOTS];
	
	int renderObjectHandle;
	
	TextureState tilemapTextureState;
	
	// Indexes into the array of room render object handles.
	// When the camera is not scrolling, current cache slot equals next cache slot.
	unsigned int currentCacheSlot;
	unsigned int nextCacheSlot;
	unsigned long long int scrollStartTimeMS;
	
	// Maps room IDs to room texture cache slots.
	unsigned int numRoomIDs;			// Equal to number of rooms.
	unsigned int *roomIDsToCacheSlots;	// UINT32_MAX represents unmapped room ID.
	Offset *roomIDsToPositions;
	
	// Maps room texture cache slots to room positions.
	// UINT32_MAX represents unmapped cache slot.
	unsigned int cacheSlotsToRoomIDs[NUM_ROOM_TEXTURE_CACHE_SLOTS];
	
} AreaRenderState;

// Resets the global area render state to reflect the given area.
void areaRenderStateReset(AreaRenderState *const pAreaRenderState, const Area area, const Room initialRoom);

// Returns true if the current cache slot and the next cache slot are not equal, false otherwise.
bool areaRenderStateIsScrolling(const AreaRenderState areaRenderState);

// Sets the next room to scroll to in the global area render state.
// Returns true if a new room texture needs to be generated, false otherwise.
bool areaRenderStateSetNextRoom(AreaRenderState *const pAreaRenderState, const Room nextRoom);

Vector4F areaRenderStateGetCameraPosition(AreaRenderState *const pAreaRenderState);

ProjectionBounds areaRenderStateGetProjectionBounds(const AreaRenderState areaRenderState);

#endif	// AREA_RENDER_STATE_H