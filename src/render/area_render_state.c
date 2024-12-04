#include "area_render_state.h"

#include "log/Logger.h"
#include "util/allocate.h"
#include "util/time.h"
#include "RenderManager.h"
#include "vulkan/texture_manager.h"
#include "vulkan/compute/ComputeStitchTexture.h"
#include "vulkan/math/lerp.h"

static const String roomXSTextureID = makeConstantString("roomXS");
static const String roomSTextureID = makeConstantString("roomS");
static const String roomMTextureID = makeConstantString("roomM");
static const String roomLTextureID = makeConstantString("roomL");

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
	logMsg(loggerRender, LOG_LEVEL_VERBOSE, "Resetting area render state...");
	
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
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error resetting area render state: failed to allocate room IDs to cache slots pointer-array.");
		return;
	}
	
	if (!allocate((void **)&pAreaRenderState->roomIDsToPositions, pAreaRenderState->numRoomIDs, sizeof(Offset))) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error resetting area render state: failed to allocate room IDs to room positions pointer array.");
		deallocate((void **)&pAreaRenderState->roomIDsToCacheSlots);
		return;
	}
	
	for (uint32_t i = 0; i < pAreaRenderState->numRoomIDs; ++i) {
		pAreaRenderState->roomIDsToCacheSlots[i] = UINT32_MAX;
		pAreaRenderState->roomIDsToPositions[i] = area.pRooms[i].position;
	}
	
	for (uint32_t i = 0; i < numRoomTextureCacheSlots; ++i) {
		pAreaRenderState->roomRenderObjHandles[i] = -1;
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
	
	logMsg(loggerRender, LOG_LEVEL_VERBOSE, "Reset area render state.");
}

bool areaRenderStateIsScrolling(const AreaRenderState areaRenderState) {
	return areaRenderState.currentCacheSlot != areaRenderState.nextCacheSlot;
}

bool areaRenderStateSetNextRoom(AreaRenderState *const pAreaRenderState, const Room nextRoom) {
	
	const unsigned int roomID = nextRoom.id;
	if (roomID >= pAreaRenderState->numRoomIDs) {	// TODO - this check may not make sense.
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error setting area render state next room: given room ID (%u) is not less than number of room IDs (%u).", roomID, pAreaRenderState->numRoomIDs);
		return false;
	}

	unsigned int nextCacheSlot = 0;
	
	// Check if the next room is already loaded.
	bool roomAlreadyLoaded = false;
	for (uint32_t i = 0; i < numRoomTextureCacheSlots; ++i) {
		if (pAreaRenderState->cacheSlotsToRoomIDs[i] == roomID) {
			roomAlreadyLoaded = true;
			nextCacheSlot = i;
			break;
		}
	}
	
	// If the room is not already loaded, find the first cache slot not being used.
	if (!roomAlreadyLoaded) {
		for (uint32_t i = 0; i < numRoomTextureCacheSlots; ++i) {
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
		return zeroVec4F;
	}
	
	static const float posZ = 0.0F;
	
	const Extent roomExtent = room_size_to_extent(pAreaRenderState->roomSize);
	const uint32_t currentRoomID = pAreaRenderState->cacheSlotsToRoomIDs[pAreaRenderState->currentCacheSlot];
	const Offset currentRoomPosition = pAreaRenderState->roomIDsToPositions[currentRoomID];
	const Vector4F start = {
		.x = (int)roomExtent.width * currentRoomPosition.x,
		.y = (int)roomExtent.length * currentRoomPosition.y,
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
		.x = (int)roomExtent.width * nextRoomPosition.x,
		.y = (int)roomExtent.length * nextRoomPosition.y,
		.z = posZ,
		.w = 1.0F
	};
	
	static const uint64_t timeLimitMS = 1024;
	const uint64_t currentTimeMS = getTimeMS();
	
	// If the scrolling time limit is reached, update the current room slot to equal the next room cache slot.
	if (currentTimeMS - pAreaRenderState->scrollStartTimeMS >= timeLimitMS) {
		pAreaRenderState->currentCacheSlot = pAreaRenderState->nextCacheSlot;
		return end;
	}
	
	const double deltaTime = (double)(currentTimeMS - pAreaRenderState->scrollStartTimeMS) / (double)(timeLimitMS);
	return lerpVec4F(start, end, deltaTime);
}

ProjectionBounds areaRenderStateGetProjectionBounds(const AreaRenderState areaRenderState) {
	const Extent roomExtent = room_size_to_extent(areaRenderState.roomSize);
	const float top = -((float)roomExtent.length / 2.0F);
	const float left = -((float)roomExtent.width / 2.0F);
	return (ProjectionBounds){
		.left = left,
		.right = -left,
		.bottom = -top,
		.top = top,
		.near = 16.0F,
		.far = -16.0F
	};
}

static void areaRenderStateLoadRoomQuad(AreaRenderState *const pAreaRenderState, const unsigned int roomCacheSlot, const Room room) {
	if (!pAreaRenderState) {
		return;
	}
	
	const Extent roomExtent = room_size_to_extent(room.size);
	
	const BoxF roomQuadDimensions = {
		.x1 = -0.5F * roomExtent.width,
		.y1 = -0.5F * roomExtent.length,
		.x2 = 0.5F * roomExtent.width,
		.y2 = 0.5F * roomExtent.length
	};
	
	const Vector3D roomLayerPositions[2] = {
		{ // Background
			(double)room.position.x * room.extent.width, 
			(double)room.position.y * room.extent.length, 
			0.0 
		}, { // Foreground
			(double)room.position.x * room.extent.width, 
			(double)room.position.y * room.extent.length, 
			2.0 
		}
	};
	
	// If there already is a room render object in the cache slot, then unload it first.
	if (renderObjectExists(pAreaRenderState->roomRenderObjHandles[roomCacheSlot])) {
		unloadRenderObject(&pAreaRenderState->roomRenderObjHandles[roomCacheSlot]);
	}
	
	const RenderObjectLoadInfo loadInfo = {
		.textureID = roomSizeToTextureID(room.size),
		.quadCount = 2,
		.pQuadLoadInfos = (QuadLoadInfo[2]){
			{
				.quadType = QUAD_TYPE_MAIN,
				.initPosition = roomLayerPositions[0],
				.quadDimensions = roomQuadDimensions,
				.initAnimation = roomCacheSlot * numRoomLayers,
				.initCell = 0,
				.color = COLOR_WHITE
			}, {
				.quadType = QUAD_TYPE_MAIN,
				.initPosition = roomLayerPositions[1],
				.quadDimensions = roomQuadDimensions,
				.initAnimation = roomCacheSlot * numRoomLayers + 1,
				.initCell = 0,
				.color = COLOR_WHITE
			}
		}
	};
	
	pAreaRenderState->roomRenderObjHandles[roomCacheSlot] = loadRenderObject(loadInfo);
	
	const int textureHandle = renderObjectGetTextureHandle(pAreaRenderState->roomRenderObjHandles[roomCacheSlot], 0);
	const ImageSubresourceRange imageSubresourceRange = {
		.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseArrayLayer = roomCacheSlot * numRoomLayers,
		.arrayLayerCount = numRoomLayers
	};
	
	computeStitchTexture(pAreaRenderState->tilemapTextureState.textureHandle, textureHandle, imageSubresourceRange, room.extent, room.ppTileIndices);
}