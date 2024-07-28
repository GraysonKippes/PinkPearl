#include "fgm_file_parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log/logging.h"
#include "render/render_config.h"
#include "util/allocate.h"
#include "util/file_io.h"

#include "area_extent.h"

#define FGA_FILE_DIRECTORY (RESOURCE_PATH "data/area2.fga")
#define FGA_FILE_DIRECTORY_2 (RESOURCE_PATH "data/DemoDungeon.fga")

static int readRoomData(FILE *const pFile, Room *const pRoom);

Area readAreaData(const char *const pFilename) {
	
	Area area = { };

	FILE *pFile = fopen(FGA_FILE_DIRECTORY_2, "rb");
	if (!pFile) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to open file.");
		return area;
	}

	// Check file label.
	char label[4];
	fread(label, 1, 4, pFile);
	if (strcmp(label, "FGA") != 0) {
		logMsgF(LOG_LEVEL_ERROR, "Error reading area file: invalid file format; found label \"%s\".", label);
		goto end_read;
	}
	
	/* Order of data reading:
	 * Area extent (4 * i32)
	 * Room size type (u32)
	 * Room count (u32)
	 * Room data
	*/
	
	// Read area extent.
	if (!read_data(pFile, sizeof(area.extent), 1, &area.extent)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read area extent.");
		goto end_read;
	}
	
	const int area_width = area_extent_width(area.extent);
	const int area_length = area_extent_length(area.extent);
	const long int extentArea = area_width * area_length;
	
	// Read room size type.
	uint32_t roomSizeType = 0;
	if (!read_data(pFile, sizeof(roomSizeType), 1, &roomSizeType)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read room size type.");
		goto end_read;
	}

	// Validate room size type, must be between 0 and 3, inclusive.
	if (roomSizeType >= num_room_sizes) {
		logMsgF(LOG_LEVEL_ERROR, "Error creating area: room size type is invalid (%u).", roomSizeType);
		goto end_read;
	}

	area.room_size = (RoomSize)roomSizeType;
	area.room_extent = room_size_to_extent(area.room_size);
	
	// Read room count.
	if (!read_data(pFile, sizeof(area.num_rooms), 1, &area.num_rooms)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read area room count.");
		goto end_read;
	}
	
	// Allocate array of rooms.
	area.pRooms = calloc(area.num_rooms, sizeof(Room));
	if (!area.pRooms) {
		logMsg(LOG_LEVEL_ERROR, "Error creating area: allocation of area.pRooms failed.");
		goto end_read;
	}

	// Allocate 1D position to room array index map.
	area.pPositionsToRooms = calloc(extentArea, sizeof(int));
	if (!area.pPositionsToRooms) {
		logMsg(LOG_LEVEL_ERROR, "Error creating area: allocation of area.pPositionsToRooms failed.");
		free(area.pRooms);
		area.pRooms = nullptr;
		goto end_read;
	}

	// Fill the position-to-room map with -1.
	for (size_t i = 0; i < (size_t)extentArea; ++i) {
		area.pPositionsToRooms[i] = -1;
	}
	
	// Set information and read data for each room in this area.
	for (uint32_t i = 0; i < area.num_rooms; ++i) {
		
		// Set basic room information.
		area.pRooms[i].id = (int)i;					// Unique numeric identifier for the room inside this area.
		area.pRooms[i].size = area.room_size;		// Copy the area room size into the room struct.
		area.pRooms[i].extent = area.room_extent;	// Copy the area room extent into the room struct.
		area.pRooms[i].num_entity_spawners = 0;		// Feature not yet implemented.
		area.pRooms[i].entity_spawners = nullptr;	// Feature not yet implemented.
		
		// Read room data.
		const int result = readRoomData(pFile, &area.pRooms[i]);
		if (result != 0) {
			logMsgF(LOG_LEVEL_ERROR, "Error creating area: error encountered while reading data for room %u (error code = %u).", i, result);
			free(area.pRooms);
			area.pRooms = nullptr;
			goto end_read;
		}

		// Map the 1D position of this room to an index into the room array inside the area struct.
		const int room_position = area_extent_index(area.extent, area.pRooms[i].position); 
		area.pPositionsToRooms[room_position] = i;
	}
	
end_read:
	return area;
}

static int readRoomData(FILE *const pFile, Room *const pRoom) {
	
	/* Order of data reading:
	 * Room Position (2 * i32)
	 * Tile Data (u32[]) for each layer
	 * Wall Count (u32)
	 * Wall Data (double[4]) for each wall
	*/
	
	// Allocate tile indices arrays.
	const uint64_t numTiles = extentArea(pRoom->extent);
	if (!allocate((void **)&pRoom->ppTileIndices, num_room_layers, sizeof(uint32_t *))) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to allocate array of tile index arrays.");
		return -1;
	}
	for (uint32_t i = 0; i < num_room_layers; ++i) {
		if (!allocate((void **)&pRoom->ppTileIndices[i], numTiles, sizeof(uint32_t))) {
			logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to allocate tile indices array.");
			return -1;
		}
	}
	
	// Read room position from file.
	if (!read_data(pFile, sizeof(Offset), 1, &pRoom->position)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read room position.");
		return -1;
	}
	
	// Read tile data for each layer from file.
	for (uint32_t i = 0; i < num_room_layers; ++i) {
		if (!read_data(pFile, sizeof(uint32_t), numTiles, pRoom->ppTileIndices[i])) {
			logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read room tile indices.");
			return -2;
		}
	}
	
	// Read wall count from the file.
	if (!read_data(pFile, sizeof(uint32_t), 1, &pRoom->num_walls)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read room wall count.");
		return -2;
	}
	
	// Allocate array for the walls.
	if (!allocate((void **)&pRoom->walls, pRoom->num_walls, sizeof(rect_t))) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to allocate room wall array.");
		return -1;
	}
	
	// Read wall data from the file.
	if (!read_data(pFile, sizeof(rect_t), pRoom->num_walls, pRoom->walls)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read room wall count.");
		return -2;
	}
	
	return 0;
}