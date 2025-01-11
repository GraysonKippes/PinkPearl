#include "fgm_file_parse.h"

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "log/Logger.h"
#include "render/render_config.h"
#include "util/Allocation.h"
#include "util/FileIO.h"

#define FGA_FILE_DIRECTORY (RESOURCE_PATH "data/DemoDungeon.fga")

static int readRoomData(const File file, Room *const pRoom);

Area readAreaData(const char *const pFilename) {
	
	Area area = { };
	File file = openFile(FGA_FILE_DIRECTORY, FMODE_READ, FMODE_NO_UPDATE, FMODE_BINARY);

	// Check file label.
	char label[4];
	fread(label, 1, 4, file.pStream);
	if (strcmp(label, "FGA") != 0) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error reading area file: invalid file format; found label \"%s\".", label);
		closeFile(&file);
		return (Area){ };
	}
	
	
	
	/* Order of data reading:
	 * Area extent (4 * i32)
	 * Room size type (u32)
	 * Room count (u32)
	 * Room data
	*/
	
	fileReadData(file, sizeof(area.extent), 1, &area.extent);
	const int areaWidth = boxWidth(area.extent);
	const int areaLength = boxLength(area.extent);
	const long int extentArea = areaWidth * areaLength;
	
	// Read room size type.
	uint32_t roomSizeType = 0;
	fileReadData(file, sizeof(roomSizeType), 1, &roomSizeType);

	// Validate room size type, must be between 0 and 3, inclusive.
	if (roomSizeType >= num_room_sizes) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error creating area: room size type is invalid (%u).", roomSizeType);
		closeFile(&file);
		return (Area){ };
	}

	area.room_size = (RoomSize)roomSizeType;
	area.room_extent = room_size_to_extent(area.room_size);
	
	// Read room count.
	fileReadData(file, sizeof(area.roomCount), 1, &area.roomCount);
	
	// Allocate array of rooms.
	area.pRooms = heapAlloc(area.roomCount, sizeof(Room));
	if (!area.pRooms) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error creating area: allocation of area.pRooms failed.");
		closeFile(&file);
		return (Area){ };
	}

	// Allocate 1D position to room array index map.
	area.pPositionsToRooms = heapAlloc(extentArea, sizeof(int));
	if (!area.pPositionsToRooms) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error creating area: allocation of area.pPositionsToRooms failed.");
		heapFree(area.pRooms);
		closeFile(&file);
		return (Area){ };
	}

	// Fill the position-to-room map with -1.
	for (long int i = 0; i < extentArea; ++i) {
		area.pPositionsToRooms[i] = -1;
	}
	
	// Set information and read data for each room in this area.
	for (int i = 0; i < area.roomCount; ++i) {
		
		// Set basic room information.
		area.pRooms[i].id = i;					// Unique numeric identifier for the room inside this area.
		area.pRooms[i].size = area.room_size;		// Copy the area room size into the room struct.
		area.pRooms[i].extent = area.room_extent;	// Copy the area room extent into the room struct.
		area.pRooms[i].num_entity_spawners = 0;		// Feature not yet implemented.
		area.pRooms[i].entity_spawners = nullptr;	// Feature not yet implemented.
		
		// Read room data.
		const int result = readRoomData(file, &area.pRooms[i]);
		if (result != 0) {
			logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error creating area: error encountered while reading data for room %u (error code = %i).", i, result);
			heapFree(area.pRooms);
			heapFree(area.pPositionsToRooms);
			closeFile(&file);
			return (Area){ };
		}

		// Map the 1D position of this room to an index into the room array inside the area struct.
		const int roomPosition = areaExtentIndex(area.extent, area.pRooms[i].position); 
		area.pPositionsToRooms[roomPosition] = i;
	}
	
	closeFile(&file);
	return area;
}

static int readRoomData(const File file, Room *const pRoom) {
	
	/* Order of data reading:
	 * Room Position (2 * i32)
	 * Tile Data (u32[]) for each layer
	 * Wall Count (u32)
	 * Wall Data (double[4]) for each wall
	*/
	
	// Allocate tile indices arrays.
	const uint64_t numTiles = extentArea(pRoom->extent);
	pRoom->ppTileIndices = heapAlloc(numRoomLayers, sizeof(uint32_t *));
	if (!pRoom->ppTileIndices) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error reading area file: failed to allocate array of tile index arrays.");
		return -1;
	}
	for (uint32_t i = 0; i < numRoomLayers; ++i) {
		pRoom->ppTileIndices[i] = heapAlloc(numTiles, sizeof(uint32_t));
		if (!pRoom->ppTileIndices[i]) {
			logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error reading area file: failed to allocate tile indices array.");
			return -1;
		}
	}
	
	// Read room position from file.
	fileReadData(file, sizeof(Offset), 1, &pRoom->position);
	
	// Read tile data for each layer from file.
	for (uint32_t i = 0; i < numRoomLayers; ++i) {
		fileReadData(file, sizeof(uint32_t), numTiles, pRoom->ppTileIndices[i]);
	}
	
	// Read wall count from the file.
	fileReadData(file, sizeof(uint32_t), 1, &pRoom->wallCount);
	
	if (pRoom->wallCount > 0) {
		// Allocate array for the walls.
		pRoom->pWalls = heapAlloc(pRoom->wallCount, sizeof(BoxD));
		if (!pRoom->pWalls) {
			logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error reading area file: failed to allocate room wall array.");
			return -1;
		}
		
		// Read wall data from the file.
		fileReadData(file, sizeof(BoxD), pRoom->wallCount, pRoom->pWalls);
	}
	
	return 0;
}