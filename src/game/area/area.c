#include "area.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "log/Logger.h"
#include "render/render_config.h"
#include "render/RenderManager.h"
#include "render/vulkan/compute/ComputeStitchTexture.h"
#include "util/time.h"

#define swap(x, y) {\
		typeof(x) tmp = x; \
		x = y; \
		y = tmp; \
		}

static const String roomXSTextureID = makeConstantString("roomXS");
static const String roomSTextureID = makeConstantString("roomS");
static const String roomMTextureID = makeConstantString("roomM");
static const String roomLTextureID = makeConstantString("roomL");

static String roomSizeToTextureID2(const RoomSize roomSize) {
	switch (roomSize) {
		case ROOM_SIZE_XS: return roomXSTextureID;
		case ROOM_SIZE_S: return roomSTextureID;
		case ROOM_SIZE_M: return roomMTextureID;
		case ROOM_SIZE_L: return roomLTextureID;
	};
	return roomMTextureID;
}

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

bool areaSetNextRoom(Area *const pArea, const CardinalDirection direction) {
	if (direction == DIRECTION_NONE) {
		return false;
	}
	
	const Room currentRoom = pArea->pRooms[pArea->currentRoomIndex];
	const Offset roomOffset = direction_offset(direction);
	const Offset nextRoomPosition = offset_add(currentRoom.position, roomOffset);
	
	Room *pNextRoom = nullptr;
	const bool result = areaGetRoom(*pArea, nextRoomPosition, &pNextRoom);
	if (!pNextRoom) {
		return false;
	}
	
	// Check if the next room is already loaded.
	uint32_t nextCacheSlot = 0;
	bool roomAlreadyLoaded = false;
	for (uint32_t i = 0; i < numRoomTextureCacheSlots; ++i) {
		if (pArea->renderState.cacheSlotsToRoomIDs[i] == (uint32_t)pNextRoom->id) {
			roomAlreadyLoaded = true;
			nextCacheSlot = i;
			break;
		}
	}
	
	// If the room is not already loaded, find the first cache slot not being used.
	if (!roomAlreadyLoaded) {
		for (uint32_t i = 0; i < numRoomTextureCacheSlots; ++i) {
			if (i != pArea->renderState.currentCacheSlot) {
				nextCacheSlot = i;
			}
		}
		
		if (pArea->renderState.nextRoomQuadIndices[0] >= 0) {
			renderObjectUnloadQuad(pArea->renderState.renderObjectHandle, &pArea->renderState.nextRoomQuadIndices[0]);
		}
		if (pArea->renderState.nextRoomQuadIndices[1] >= 0) {
			renderObjectUnloadQuad(pArea->renderState.renderObjectHandle, &pArea->renderState.nextRoomQuadIndices[1]);
		}
		
		const BoxF roomQuadDimensions = {
			.x1 = -0.5F * pArea->room_extent.width,
			.y1 = -0.5F * pArea->room_extent.length,
			.x2 = 0.5F * pArea->room_extent.width,
			.y2 = 0.5F * pArea->room_extent.length
		};
		const Vector3D roomLayerPositions[2] = {
			{ // Background
				(double)pNextRoom->position.x * pArea->room_extent.width, 
				(double)pNextRoom->position.y * pArea->room_extent.length, 
				0.0
			}, { // Foreground
				(double)pNextRoom->position.x * pArea->room_extent.width, 
				(double)pNextRoom->position.y * pArea->room_extent.length, 
				2.0
			}
		};
		const QuadLoadInfo quadLoadInfos[2] = {
			{	// Background
				.quadType = QUAD_TYPE_MAIN,
				.initPosition = roomLayerPositions[0],
				.quadDimensions = roomQuadDimensions,
				.initAnimation = nextCacheSlot * numRoomLayers,
				.initCell = 0,
				.color = COLOR_WHITE
			}, { // Foreground
				.quadType = QUAD_TYPE_MAIN,
				.initPosition = roomLayerPositions[1],
				.quadDimensions = roomQuadDimensions,
				.initAnimation = nextCacheSlot * numRoomLayers + 1,
				.initCell = 0,
				.color = COLOR_WHITE
			}
		};
		pArea->renderState.nextRoomQuadIndices[0] = renderObjectLoadQuad(pArea->renderState.renderObjectHandle, quadLoadInfos[0]);
		pArea->renderState.nextRoomQuadIndices[1] = renderObjectLoadQuad(pArea->renderState.renderObjectHandle, quadLoadInfos[1]);
		
		const int32_t textureHandle = renderObjectGetTextureHandle(pArea->renderState.renderObjectHandle, 0);
		const ImageSubresourceRange imageSubresourceRange = {
			.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseArrayLayer = nextCacheSlot * numRoomLayers,
			.arrayLayerCount = numRoomLayers
		};
		computeStitchTexture(pArea->renderState.tilemapTextureState.textureHandle, textureHandle, imageSubresourceRange, pArea->room_extent, pNextRoom->ppTileIndices);
	} else {
		swap(pArea->renderState.currentRoomQuadIndices[0], pArea->renderState.nextRoomQuadIndices[0]);
		swap(pArea->renderState.currentRoomQuadIndices[1], pArea->renderState.nextRoomQuadIndices[1]);
	}
	pArea->renderState.nextCacheSlot = nextCacheSlot;
	pArea->renderState.scrollStartTimeMS = getTimeMS();
	pArea->renderState.roomIDsToCacheSlots[pNextRoom->id] = pArea->renderState.nextCacheSlot;
	pArea->renderState.cacheSlotsToRoomIDs[pArea->renderState.nextCacheSlot] = pNextRoom->id;
	pArea->currentRoomIndex = pNextRoom->id;
	
	
	
	/*const RenderObjectLoadInfo loadInfo = {
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
	
	// If there already is a room render object in the cache slot, then unload it first.
	if (renderObjectExists(pAreaRenderState->roomRenderObjHandles[roomCacheSlot])) {
		unloadRenderObject(&pAreaRenderState->roomRenderObjHandles[roomCacheSlot]);
	}*/
	
	
	
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

void areaRenderStateReset(Area *const pArea, const Room initialRoom) {
	logMsg(loggerRender, LOG_LEVEL_VERBOSE, "Resetting area render state...");
	
	const String tilemapTextureID = makeStaticString("tilemap/dungeon4");
	pArea->renderState.tilemapTextureState = newTextureState(tilemapTextureID);
	pArea->renderState.numRoomIDs = pArea->roomCount;
	pArea->renderState.roomIDsToCacheSlots = realloc(pArea->renderState.roomIDsToCacheSlots, pArea->renderState.numRoomIDs * sizeof(uint32_t));
	pArea->renderState.roomIDsToPositions = realloc(pArea->renderState.roomIDsToPositions, pArea->renderState.numRoomIDs * sizeof(uint32_t));
	
	if (!pArea->renderState.roomIDsToCacheSlots) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error resetting area render state: failed to allocate room IDs to cache slots pointer-array.");
		free(pArea->renderState.roomIDsToPositions);
		return;
	}
	
	if (!pArea->renderState.roomIDsToPositions) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error resetting area render state: failed to allocate room IDs to room positions pointer array.");
		free(pArea->renderState.roomIDsToCacheSlots);
		return;
	}
	
	for (uint32_t i = 0; i < pArea->renderState.numRoomIDs; ++i) {
		pArea->renderState.roomIDsToCacheSlots[i] = UINT32_MAX;
		pArea->renderState.roomIDsToPositions[i] = pArea->pRooms[i].position;
	}
	
	for (uint32_t i = 0; i < numRoomTextureCacheSlots; ++i) {
		//pArea->renderState.roomRenderObjHandles[i] = -1;
		pArea->renderState.cacheSlotsToRoomIDs[i] = UINT32_MAX;
	}
	
	pArea->renderState.currentCacheSlot = 0;
	pArea->renderState.nextCacheSlot = 0;
	pArea->renderState.roomIDsToCacheSlots[initialRoom.id] = pArea->renderState.currentCacheSlot;
	pArea->renderState.roomIDsToPositions[initialRoom.id] = initialRoom.position;
	pArea->renderState.cacheSlotsToRoomIDs[pArea->renderState.currentCacheSlot] = (uint32_t)initialRoom.id;
	
	//areaRenderStateLoadRoomQuad(pAreaRenderState, pAreaRenderState->currentCacheSlot, initialRoom);
	
	const BoxF roomQuadDimensions = {
		.x1 = -0.5F * pArea->room_extent.width,
		.y1 = -0.5F * pArea->room_extent.length,
		.x2 = 0.5F * pArea->room_extent.width,
		.y2 = 0.5F * pArea->room_extent.length
	};
	const Vector3D roomLayerPositions[NUM_ROOM_LAYERS] = {
		{ // Background
			(double)initialRoom.position.x * pArea->room_extent.width, 
			(double)initialRoom.position.y * pArea->room_extent.length, 
			0.0 
		}, { // Foreground
			(double)initialRoom.position.x * pArea->room_extent.width, 
			(double)initialRoom.position.y * pArea->room_extent.length, 
			2.0 
		}
	};
	const RenderObjectLoadInfo renderObjectLoadInfo = {
		.textureID = roomSizeToTextureID2(initialRoom.size),
		.quadCount = NUM_ROOM_LAYERS,
		.pQuadLoadInfos = (QuadLoadInfo[NUM_ROOM_LAYERS]){
			{
				.quadType = QUAD_TYPE_MAIN,
				.initPosition = roomLayerPositions[0],
				.quadDimensions = roomQuadDimensions,
				.initAnimation = pArea->renderState.currentCacheSlot * numRoomLayers,
				.initCell = 0,
				.color = COLOR_WHITE
			}, {
				.quadType = QUAD_TYPE_MAIN,
				.initPosition = roomLayerPositions[1],
				.quadDimensions = roomQuadDimensions,
				.initAnimation = pArea->renderState.currentCacheSlot * numRoomLayers + 1,
				.initCell = 0,
				.color = COLOR_WHITE
			}
		}
	};
	pArea->renderState.renderObjectHandle = loadRenderObject(renderObjectLoadInfo);
	pArea->renderState.currentRoomQuadIndices[0] = 0;
	pArea->renderState.currentRoomQuadIndices[1] = 1;
	pArea->renderState.nextRoomQuadIndices[0] = -1;
	pArea->renderState.nextRoomQuadIndices[1] = -1;
	
	const int32_t textureHandle = renderObjectGetTextureHandle(pArea->renderState.renderObjectHandle, 0);
	const ImageSubresourceRange imageSubresourceRange = {
		.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseArrayLayer = pArea->renderState.currentCacheSlot * numRoomLayers,
		.arrayLayerCount = numRoomLayers
	};
	computeStitchTexture(pArea->renderState.tilemapTextureState.textureHandle, textureHandle, imageSubresourceRange, pArea->room_extent, initialRoom.ppTileIndices);
	
	logMsg(loggerRender, LOG_LEVEL_VERBOSE, "Reset area render state.");
}

bool areaIsScrolling(const Area area) {
	return area.renderState.currentCacheSlot != area.renderState.nextCacheSlot;
}

Vector4F areaGetCameraPosition(Area *const pArea) {
	static const float posZ = 0.0F;
	
	const uint32_t currentRoomID = pArea->renderState.cacheSlotsToRoomIDs[pArea->renderState.currentCacheSlot];
	const Offset currentRoomPosition = pArea->renderState.roomIDsToPositions[currentRoomID];
	const Vector4F start = {
		.x = (int)pArea->room_extent.width * currentRoomPosition.x,
		.y = (int)pArea->room_extent.length * currentRoomPosition.y,
		.z = posZ,
		.w = 1.0F
	};
	
	// If the area render state is not in scrolling state, just return the starting camera position.
	if (!areaIsScrolling(*pArea)) {
		return start;
	}
	
	const uint32_t nextRoomID = pArea->renderState.cacheSlotsToRoomIDs[pArea->renderState.nextCacheSlot];
	const Offset nextRoomPosition = pArea->renderState.roomIDsToPositions[nextRoomID];
	const Vector4F end = {
		.x = (int)pArea->room_extent.width * nextRoomPosition.x,
		.y = (int)pArea->room_extent.length * nextRoomPosition.y,
		.z = posZ,
		.w = 1.0F
	};
	
	static const uint64_t timeLimitMS = 1024;
	const uint64_t currentTimeMS = getTimeMS();
	
	// If the scrolling time limit is reached, update the current room slot to equal the next room cache slot.
	if (currentTimeMS - pArea->renderState.scrollStartTimeMS >= timeLimitMS) {
		pArea->renderState.currentCacheSlot = pArea->renderState.nextCacheSlot;
		swap(pArea->renderState.currentRoomQuadIndices[0], pArea->renderState.nextRoomQuadIndices[0]);
		swap(pArea->renderState.currentRoomQuadIndices[1], pArea->renderState.nextRoomQuadIndices[1]);
		return end;
	}
	
	const double deltaTime = (double)(currentTimeMS - pArea->renderState.scrollStartTimeMS) / (double)(timeLimitMS);
	return lerpVec4F(start, end, deltaTime);
}

ProjectionBounds areaGetProjectionBounds(const Area area) {
	const float top = -((float)area.room_extent.length / 2.0F);
	const float left = -((float)area.room_extent.width / 2.0F);
	return (ProjectionBounds){
		.left = left,
		.right = -left,
		.bottom = -top,
		.top = top,
		.near = 16.0F,
		.far = -16.0F
	};
}