#include "area_render_state.h"

#include "log/logging.h"
#include "util/allocate.h"
#include "util/time.h"

#include "render_object.h"
#include "vulkan/texture_manager.h"
#include "vulkan/vulkan_render.h"
#include "vulkan/compute/compute_room_texture.h"
#include "vulkan/math/lerp.h"

static const String roomXSTextureID = {
	.length = 6,
	.capacity = 7,
	.buffer = "roomXS"
};

static const String roomSTextureID = {
	.length = 5,
	.capacity = 6,
	.buffer = "roomS"
};

static const String roomMTextureID = {
	.length = 5,
	.capacity = 6,
	.buffer = "roomM"
};

static const String roomLTextureID = {
	.length = 5,
	.capacity = 6,
	.buffer = "roomL"
};

static String roomSizeToTextureID(const RoomSize roomSize) {
	switch (roomSize) {
		case ROOM_SIZE_XS: return roomXSTextureID;
		case ROOM_SIZE_S: return roomSTextureID;
		case ROOM_SIZE_M: return roomMTextureID;
		case ROOM_SIZE_L: return roomLTextureID;
	};
	return roomMTextureID;
}

static void areaRenderStateLoadRoomQuad(AreaRenderState *const pAreaRenderState, const uint32_t cacheSlot, const Room room);

void areaRenderStateReset(AreaRenderState *const pAreaRenderState, const Area area, const Room initialRoom) {
	logMsg(LOG_LEVEL_VERBOSE, "Resetting area render state...");
	
	if (!pAreaRenderState) {
		return;
	}
	
	String tilemapTextureID = newString(64, "tilemap/dungeon4");
	pAreaRenderState->tilemapTextureState = newTextureState(tilemapTextureID);
	deleteString(&tilemapTextureID);
	
	// TODO - use reallocate function when it is implemented.
	deallocate((void **)&pAreaRenderState->roomIDsToPositions);
	pAreaRenderState->numRoomIDs = area.roomCount;
	
	if (!allocate((void **)&pAreaRenderState->roomIDsToCacheSlots, pAreaRenderState->numRoomIDs, sizeof(uint32_t))) {
		logMsg(LOG_LEVEL_ERROR, "Error resetting area render state: failed to allocate room IDs to cache slots pointer-array.");
		return;
	}
	
	if (!allocate((void **)&pAreaRenderState->roomIDsToPositions, pAreaRenderState->numRoomIDs, sizeof(Offset))) {
		logMsg(LOG_LEVEL_ERROR, "Error resetting area render state: failed to allocate room IDs to room positions pointer array.");
		deallocate((void **)&pAreaRenderState->roomIDsToCacheSlots);
		return;
	}
	
	for (uint32_t i = 0; i < pAreaRenderState->numRoomIDs; ++i) {
		pAreaRenderState->roomIDsToCacheSlots[i] = UINT32_MAX;
	}
	
	for (uint32_t i = 0; i < pAreaRenderState->numRoomIDs; ++i) {
		pAreaRenderState->roomIDsToPositions[i] = area.pRooms[i].position;
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
	
	areaRenderStateLoadRoomQuad(pAreaRenderState, pAreaRenderState->currentCacheSlot, initialRoom);
	
	logMsg(LOG_LEVEL_VERBOSE, "Done resetting area render state.");
}

bool areaRenderStateIsScrolling(const AreaRenderState areaRenderState) {
	return areaRenderState.currentCacheSlot != areaRenderState.nextCacheSlot;
}

bool areaRenderStateSetNextRoom(AreaRenderState *const pAreaRenderState, const Room nextRoom) {
	
	const unsigned int roomID = nextRoom.id;
	if (roomID >= pAreaRenderState->numRoomIDs) {	// TODO - this check may not make sense.
		logMsgF(LOG_LEVEL_ERROR, "Error setting area render state next room: given room ID (%u) is not less than number of room IDs (%u).", roomID, pAreaRenderState->numRoomIDs);
		return false;
	}

	unsigned int nextCacheSlot = 0;
	
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
	
	if (!roomAlreadyLoaded) {
		areaRenderStateLoadRoomQuad(pAreaRenderState, pAreaRenderState->nextCacheSlot, nextRoom);
	}
	
	return !roomAlreadyLoaded;
}

Vector4F areaRenderStateGetCameraPosition(AreaRenderState *const pAreaRenderState) {
	if (!pAreaRenderState) {
		return zeroVector4F;
	}
	
	static const float posZ = 0.0F;
	
	const Extent roomExtent = room_size_to_extent(pAreaRenderState->roomSize);
	const uint32_t currentRoomID = pAreaRenderState->cacheSlotsToRoomIDs[pAreaRenderState->currentCacheSlot];
	const Offset currentRoomPosition = pAreaRenderState->roomIDsToPositions[currentRoomID];
	const Vector4F start = {
		.x = roomExtent.width * currentRoomPosition.x,
		.y = roomExtent.length * currentRoomPosition.y,
		.z = posZ,
		.w = 1.0F
	};
	
	// If the area render state is not in scrolling state, just return the starting camera position.
	if (!areaRenderStateIsScrolling(*pAreaRenderState)) {
		return start;
	}
	
	const uint32_t nextRoomID = pAreaRenderState->cacheSlotsToRoomIDs[pAreaRenderState->nextCacheSlot];
	const Offset nextRoomPosition = pAreaRenderState->roomIDsToPositions[nextRoomID];
	const Vector4F end = {
		.x = roomExtent.width * nextRoomPosition.x,
		.y = roomExtent.length * nextRoomPosition.y,
		.z = posZ,
		.w = 1.0F
	};
	
	static const uint64_t timeLimitMS = 1024;
	const uint64_t currentTimeMS = getTimeMS();
	
	// If the scrolling time limit is reached, update the current room slot to equal the next room cache slot.
	if (currentTimeMS - pAreaRenderState->scrollStartTimeMS >= timeLimitMS) {
		unloadRenderObject(&pAreaRenderState->roomRenderObjHandles[pAreaRenderState->currentCacheSlot]);
		pAreaRenderState->currentCacheSlot = pAreaRenderState->nextCacheSlot;
		return end;
	}
	
	const double deltaTime = (double)(currentTimeMS - pAreaRenderState->scrollStartTimeMS) / (double)(timeLimitMS);
	return vector4F_lerp(start, end, deltaTime);
}

ProjectionBounds areaRenderStateGetProjectionBounds(const AreaRenderState areaRenderState) {
	const Extent roomExtent = room_size_to_extent(areaRenderState.roomSize);
	const float top = -((float)roomExtent.length / 2.0F);
	const float left = -((float)roomExtent.width / 2.0F);
	const float bottom = -top;
	const float right = -left;
	return (ProjectionBounds){
		.left = left,
		.right = right,
		.bottom = bottom,
		.top = top,
		.near = -64.0F,
		.far = 64.0F
	};
}

static void areaRenderStateLoadRoomQuad(AreaRenderState *const pAreaRenderState, const uint32_t cacheSlot, const Room room) {
	
	const Extent roomExtent = room_size_to_extent(room.size);
	const DimensionsF roomQuadDimensions = {
		.x1 = -0.5F * roomExtent.width,
		.y1 = -0.5F * roomExtent.length,
		.x2 = 0.5F * roomExtent.width,
		.y2 = 0.5F * roomExtent.length
	};
	
	const Vector3D roomLayerPositions[2] = {
		{ // Background
			(double)room.position.x * room.extent.width, 
			(double)room.position.y * room.extent.length, 
			-16.0 
		},
		{ // Foreground
			(double)room.position.x * room.extent.width, 
			(double)room.position.y * room.extent.length, 
			-48.0 
		}
	};
	
	pAreaRenderState->roomRenderObjHandles[cacheSlot] = loadRenderObject(roomSizeToTextureID(room.size), roomQuadDimensions, 2, roomLayerPositions);
	
	const int textureHandle = renderObjectGetTextureHandle(pAreaRenderState->roomRenderObjHandles[cacheSlot], 0);
	const ImageSubresourceRange imageSubresourceRange = {
		.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseArrayLayer = cacheSlot * num_room_layers,
		.arrayLayerCount = num_room_layers
	};
	
	computeStitchTexture(pAreaRenderState->tilemapTextureState.textureHandle, textureHandle, imageSubresourceRange, room.extent, room.ppTileIndices);
	//renderObjectSetAnimation(pAreaRenderState->roomRenderObjHandles[cacheSlot], 0, 0);
	//renderObjectSetAnimation(pAreaRenderState->roomRenderObjHandles[cacheSlot], 1, 1);
	renderObjectSetAnimation(pAreaRenderState->roomRenderObjHandles[cacheSlot], 0, cacheSlot * num_room_layers);
	renderObjectSetAnimation(pAreaRenderState->roomRenderObjHandles[cacheSlot], 1, cacheSlot * num_room_layers + 1);
}