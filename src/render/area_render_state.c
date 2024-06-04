#include "area_render_state.h"

#include "log/logging.h"
#include "util/allocate.h"
#include "util/time.h"

#include "render_object.h"
#include "vulkan/texture_manager.h"
#include "vulkan/vulkan_render.h"
#include "vulkan/math/lerp.h"

void areaRenderStateReset(AreaRenderState *const pAreaRenderState, const area_t area, const room_t initialRoom) {
	log_message(VERBOSE, "Resetting area render state...");
	
	if (pAreaRenderState == NULL) {
		return;
	}
	
	String tilemapTextureID = newString(64, "tilemap/dungeon4");
	pAreaRenderState->tilemapTextureState = newTextureState(tilemapTextureID);
	deleteString(&tilemapTextureID);
	
	// TODO - use reallocate function when it is implemented.
	deallocate((void **)&pAreaRenderState->roomIDsToPositions);
	pAreaRenderState->numRoomIDs = area.num_rooms;
	
	if (!allocate((void **)&pAreaRenderState->roomIDsToCacheSlots, pAreaRenderState->numRoomIDs, sizeof(uint32_t))) {
		log_message(ERROR, "Error resetting area render state: failed to allocate room IDs to cache slots pointer-array.");
		return;
	}
	
	if (!allocate((void **)&pAreaRenderState->roomIDsToPositions, pAreaRenderState->numRoomIDs, sizeof(offset_t))) {
		log_message(ERROR, "Error resetting area render state: failed to allocate room IDs to room positions pointer array.");
		deallocate((void **)&pAreaRenderState->roomIDsToCacheSlots);
		return;
	}
	
	for (uint32_t i = 0; i < pAreaRenderState->numRoomIDs; ++i) {
		pAreaRenderState->roomIDsToCacheSlots[i] = UINT32_MAX;
	}
	
	for (uint32_t i = 0; i < pAreaRenderState->numRoomIDs; ++i) {
		pAreaRenderState->roomIDsToPositions[i] = area.rooms[i].position;
	}
	
	for (uint32_t i = 0; i < num_room_texture_cache_slots; ++i) {
		pAreaRenderState->cacheSlotsToRoomIDs[i] = UINT32_MAX;
	}
	
	pAreaRenderState->currentCacheSlot = 0;
	pAreaRenderState->nextCacheSlot = 0;
	
	pAreaRenderState->roomIDsToCacheSlots[initialRoom.id] = pAreaRenderState->currentCacheSlot;
	pAreaRenderState->roomIDsToPositions[initialRoom.id] = initialRoom.position;
	pAreaRenderState->cacheSlotsToRoomIDs[pAreaRenderState->currentCacheSlot] = (uint32_t)initialRoom.id;
	
	pAreaRenderState->areaExtent = area.extent;
	pAreaRenderState->roomSize = area.room_size;
	
	//create_room_texture(initialRoom, pAreaRenderState->currentCacheSlot, pAreaRenderState->tilemapTextureState.textureHandle);
	
	const extent_t roomExtent = room_size_to_extent(pAreaRenderState->roomSize);
	const DimensionsF roomQuadDimensions = {
		.x1 = -0.5F * roomExtent.width,
		.y1 = 0.5F * roomExtent.length,
		.x2 = 0.5F * roomExtent.width,
		.y2 = -0.5F * roomExtent.length
	};
	
	const Transform roomQuadTransform = {
		.translation = (Vector4F){ 0.0F, 0.0F, 0.0F, 1.0F },
		.scaling = zeroVector4F,
		.rotation = zeroVector4F
	};
	
	// Testing
	//String textureID = newString(64, "roomM");
	//pAreaRenderState->roomRenderObjHandles[pAreaRenderState->currentCacheSlot] = loadRenderObject(roomQuadDimensions, roomQuadTransform, textureID);
	//deleteString(&textureID);
	
	log_message(VERBOSE, "Done resetting area render state.");
}

bool areaRenderStateIsScrolling(const AreaRenderState areaRenderState) {
	return areaRenderState.currentCacheSlot != areaRenderState.nextCacheSlot;
}

bool areaRenderStateSetNextRoom(AreaRenderState *const pAreaRenderState, const room_t nextRoom) {
	
	const uint32_t roomID = (uint32_t)nextRoom.id;
	
	if (roomID >= pAreaRenderState->numRoomIDs) {
		logf_message(ERROR, "Error setting area render state next room: given room ID (%u) is not less than number of room IDs (%u).", roomID, pAreaRenderState->numRoomIDs);
		return false;
	}
	
	uint32_t nextCacheSlot = 0;
	
	// Check if the next room is already loaded.
	bool roomAlreadyLoaded = false;
	for (uint32_t i = 0; i < num_room_texture_cache_slots; ++i) {
		if (pAreaRenderState->cacheSlotsToRoomIDs[i] == roomID) {
			roomAlreadyLoaded = true;
			nextCacheSlot = i;
			break;
		}
	}
	
	// If the room is not already loaded, find the first cache slot not being used.
	if (!roomAlreadyLoaded) {
		for (uint32_t i = 0; i < num_room_texture_cache_slots; ++i) {
			if (i != pAreaRenderState->currentCacheSlot) {
				nextCacheSlot = i;
			}
		}
	}
	
	pAreaRenderState->nextCacheSlot = nextCacheSlot;
	pAreaRenderState->scrollStartTimeMS = getTimeMS();
	
	pAreaRenderState->roomIDsToCacheSlots[roomID] = pAreaRenderState->nextCacheSlot;
	pAreaRenderState->cacheSlotsToRoomIDs[pAreaRenderState->nextCacheSlot] = roomID;
	
	// Update Vulkan engine.
	// TODO - move all functionality to render layer.
	//upload_draw_data(*pAreaRenderState);
	if (!roomAlreadyLoaded) {
		//create_room_texture(nextRoom, pAreaRenderState->nextCacheSlot, pAreaRenderState->tilemapTextureState.textureHandle);
	}
	
	return !roomAlreadyLoaded;
}

Vector4F areaRenderStateGetCameraPosition(AreaRenderState *const pAreaRenderState) {
	if (pAreaRenderState == NULL) {
		return zeroVector4F;
	}
	
	const extent_t roomExtent = room_size_to_extent(pAreaRenderState->roomSize);
	const uint32_t currentRoomID = pAreaRenderState->cacheSlotsToRoomIDs[pAreaRenderState->currentCacheSlot];
	const offset_t currentRoomPosition = pAreaRenderState->roomIDsToPositions[currentRoomID];
	const Vector4F start = {
		.x = roomExtent.width * currentRoomPosition.x,
		.y = roomExtent.length * currentRoomPosition.y,
		.z = 0.0F,
		.w = 1.0F
	};
	
	// If the area render state is not in scrolling state, just return the starting camera position.
	if (!areaRenderStateIsScrolling(*pAreaRenderState)) {
		return start;
	}
	
	const uint32_t nextRoomID = pAreaRenderState->cacheSlotsToRoomIDs[pAreaRenderState->nextCacheSlot];
	const offset_t nextRoomPosition = pAreaRenderState->roomIDsToPositions[nextRoomID];
	const Vector4F end = {
		.x = roomExtent.width * nextRoomPosition.x,
		.y = roomExtent.length * nextRoomPosition.y,
		.z = 0.0F,
		.w = 1.0F
	};
	
	static const uint64_t timeLimitMS = 1024;
	const uint64_t currentTimeMS = getTimeMS();
	
	if (currentTimeMS - pAreaRenderState->scrollStartTimeMS >= timeLimitMS) {
		pAreaRenderState->currentCacheSlot = pAreaRenderState->nextCacheSlot;
		return end;
	}
	
	const double deltaTime = (double)(currentTimeMS - pAreaRenderState->scrollStartTimeMS) / (double)(timeLimitMS);
	return vector4F_lerp(start, end, deltaTime);
}

projection_bounds_t areaRenderStateGetProjectionBounds(const AreaRenderState areaRenderState) {
	const extent_t roomExtent = room_size_to_extent(areaRenderState.roomSize);
	const float top = -((float)roomExtent.length / 2.0F);
	const float left = -((float)roomExtent.width / 2.0F);
	const float bottom = -top;
	const float right = -left;
	return (projection_bounds_t){
		.left = left,
		.right = right,
		.bottom = bottom,
		.top = top,
		.near = 15.0F,
		.far = -15.0F
	};
}