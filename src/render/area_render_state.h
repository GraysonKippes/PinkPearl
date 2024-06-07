#ifndef AREA_RENDER_STATE_H
#define AREA_RENDER_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "game/area/area.h"
#include "game/area/area_extent.h"
#include "game/area/room.h"
#include "math/offset.h"
#include "vulkan/math/projection.h"
#include "vulkan/math/vector4F.h"

#include "render_config.h"
#include "texture_state.h"

typedef struct AreaRenderState {
	
	area_extent_t areaExtent;
	RoomSize roomSize;
	
	int roomRenderObjHandles[NUM_ROOM_TEXTURE_CACHE_SLOTS];
	
	TextureState tilemapTextureState;
	
	// When the camera is not scrolling, current cache slot equals next cache slot.
	unsigned int currentCacheSlot;
	unsigned int nextCacheSlot;
	unsigned long long int scrollStartTimeMS;
	
	// Maps room IDs to room texture cache slots.
	unsigned int numRoomIDs;	// Equal to number of rooms.
	unsigned int *roomIDsToCacheSlots;	// UINT32_MAX represents unmapped room ID.
	offset_t *roomIDsToPositions;
	
	// Maps room texture cache slots to room positions.
	// UINT32_MAX represents unmapped cache slot.
	unsigned int cacheSlotsToRoomIDs[NUM_ROOM_TEXTURE_CACHE_SLOTS];
	
} AreaRenderState;

// Resets the global area render state to reflect the given area.
void areaRenderStateReset(AreaRenderState *const pAreaRenderState, const area_t area, const Room initialRoom);

// Returns true if the current cache slot and the next cache slot are equal, false otherwise.
bool areaRenderStateIsScrolling(const AreaRenderState areaRenderState);

// Sets the next room to scroll to in the global area render state.
// Returns true if a new room texture needs to be generated, false otherwise.
bool areaRenderStateSetNextRoom(AreaRenderState *const pAreaRenderState, const Room nextRoom);

Vector4F areaRenderStateGetCameraPosition(AreaRenderState *const pAreaRenderState);

projection_bounds_t areaRenderStateGetProjectionBounds(const AreaRenderState areaRenderState);

#endif	// AREA_RENDER_STATE_H