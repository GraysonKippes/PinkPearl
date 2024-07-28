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

static int readRoomData(FILE *file, Room *const pRoom);
static void transposeTileData(const Extent extent, const Tile *const pTiles, uint16_t **ppTileIndices);

static int readRoomData2(FILE *const pFile, Room *const pRoom);

area_t parse_fga_file(const char *const filename) {

	area_t area = { };

	FILE *fga_file = fopen(FGA_FILE_DIRECTORY, "rb");
	if (fga_file == nullptr) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to open file.");
		return area;
	}

	// Check file label.
	char label[4];
	fread(label, 1, 4, fga_file);
	if (strcmp(label, "FGA") != 0) {
		logMsgF(LOG_LEVEL_ERROR, "Error reading area file: invalid file format; found label \"%s\".", label);
		goto end_read;
	}
	fseek(fga_file, 0x10, SEEK_SET);

	// Read area extent.
	if (!read_data(fga_file, sizeof(area.extent), 1, &area.extent)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read area extent.");
		goto end_read;
	}

	// Compute and validate area dimensions.
	const int area_width = area_extent_width(area.extent);
	const int area_length = area_extent_length(area.extent);
	const long int extentArea = area_width * area_length;
	if (area_width < 1) {
		logMsgF(LOG_LEVEL_ERROR, "Error creating area: the width of the area is not positive (%i).", area_width);
		goto end_read;
	}
	if (area_length < 1) {
		logMsgF(LOG_LEVEL_ERROR, "Error creating area: the length of the area is not positive (%i).", area_length);
		goto end_read;
	}

	fseek(fga_file, 0x20, SEEK_SET);

	// Read room extent type.
	uint32_t room_extent_type = 0;
	if (!read_data(fga_file, 4, 1, &room_extent_type)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read room extent type.");
		goto end_read;
	}

	if (room_extent_type >= num_room_sizes) {
		logMsgF(LOG_LEVEL_ERROR, "Error creating area: room extent type is invalid value (%u).", room_extent_type);
		goto end_read;
	}

	const RoomSize room_size = (RoomSize)room_extent_type;
	area.room_size = room_size;
	area.room_extent = room_size_to_extent(room_size);

	// Read number of rooms.
	if (!read_data(fga_file, 4, 1, &area.num_rooms)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read number of rooms.");
		goto end_read;
	}

	// Validate room count.
	if ((int64_t)area.num_rooms > extentArea) {
		logMsgF(LOG_LEVEL_ERROR, "Error reading area fga_file: `area.num_rooms` (%u) is greater than `area.extent.width` times `area.extent.height` (%lu).", area.num_rooms, extentArea);
		goto end_read;
	}

	// Allocate array of rooms.
	area.rooms = calloc(area.num_rooms, sizeof(Room));
	if (!area.rooms) {
		logMsg(LOG_LEVEL_ERROR, "Error creating area: allocation of area.rooms failed.");
		goto end_read;
	}

	// Allocate 1D position to room array index map.
	area.positions_to_rooms = calloc(extentArea, sizeof(int));
	if (!area.positions_to_rooms) {
		logMsg(LOG_LEVEL_ERROR, "Error creating area: allocation of area.positions_to_rooms failed.");
		free(area.rooms);
		area.rooms = nullptr;
		goto end_read;
	}

	// Fill the position-to-room map with -1.
	for (size_t i = 0; i < (size_t)extentArea; ++i) {
		area.positions_to_rooms[i] = -1;
	}

	// Read room data.
	fseek(fga_file, 0x30, SEEK_SET);
	for (uint32_t i = 0; i < area.num_rooms; ++i) {

		area.rooms[i].id = (int)i;
		area.rooms[i].size = area.room_size;
		area.rooms[i].extent = area.room_extent;
		area.rooms[i].num_entity_spawners = 0;
		area.rooms[i].entity_spawners = nullptr;

		const int result = readRoomData(fga_file, &area.rooms[i]);
		if (result != 0) {
			logMsgF(LOG_LEVEL_ERROR, "Error creating area: error encountered while reading data for room %u (error code = %u).", i, result);
			free(area.rooms);
			area.rooms = nullptr;
			goto end_read;
		}

		const int room_position = area_extent_index(area.extent, area.rooms[i].position); 
		area.positions_to_rooms[room_position] = i;
	}

end_read:
	fclose(fga_file);
	return area;
}

static int readRoomData(FILE *file, Room *const pRoom) {
	
	// Must not close file.
	
	const uint64_t num_tiles = extentArea(pRoom->extent);

	if (!read_data(file, sizeof(Offset), 1, &pRoom->position)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read room position.");
		return -1;
	}
	
	pRoom->ppTileIndices = calloc(num_room_layers, sizeof(uint16_t *));
	for (uint32_t i = 0; i < num_room_layers; ++i) {
		pRoom->ppTileIndices[i] = calloc(num_tiles, sizeof(uint16_t));
	}
	
	Tile *pTiles = calloc(num_tiles, sizeof(Tile));
	if (!pTiles) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to allocate tile array.");
		return -2;
	}

	// Read wall information.
	uint32_t num_walls = 0;
	fread(&num_walls, 4, 1, file);
	fseek(file, 4, SEEK_CUR);
	if (num_walls > 0) {

		pRoom->num_walls = num_walls;
		pRoom->walls = calloc(num_walls, sizeof(rect_t));
		if (!pRoom->walls) {
			logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to allocate wall array.");
			return -3;
		}

		if (!read_data(file, sizeof(rect_t), pRoom->num_walls, pRoom->walls)) {
			logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read room wall data.");
			return -4;
		}
	}

	// Read tile information.
	if (!read_data(file, sizeof(Tile), num_tiles, pTiles)) {
		logMsg(LOG_LEVEL_ERROR, "Error reading area file: failed to read room tile data.");
		return -5;
	}
	//transposeTileData(pRoom->extent, pTiles, pRoom->ppTileIndices);

	return 0;
}

// Temporary function to transpose tile data into layer-by-layer arrays of tile indices.
static void transposeTileData(const Extent extent, const Tile *const pTiles, uint16_t **ppTileIndices) {
	if (!pTiles || !ppTileIndices) {
		return;
	}
	
	const uint64_t numTiles = extentArea(extent);
	for (uint64_t i = 0; i < numTiles; ++i) {
		ppTileIndices[0][i] = pTiles[i].background_tilemap_slot;
		ppTileIndices[1][i] = pTiles[i].foreground_tilemap_slot;
	}
}

area_t readAreaData(const char *const pFilename) {
	
	area_t area = { };

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
	area.rooms = calloc(area.num_rooms, sizeof(Room));
	if (!area.rooms) {
		logMsg(LOG_LEVEL_ERROR, "Error creating area: allocation of area.rooms failed.");
		goto end_read;
	}

	// Allocate 1D position to room array index map.
	area.positions_to_rooms = calloc(extentArea, sizeof(int));
	if (!area.positions_to_rooms) {
		logMsg(LOG_LEVEL_ERROR, "Error creating area: allocation of area.positions_to_rooms failed.");
		free(area.rooms);
		area.rooms = nullptr;
		goto end_read;
	}

	// Fill the position-to-room map with -1.
	for (size_t i = 0; i < (size_t)extentArea; ++i) {
		area.positions_to_rooms[i] = -1;
	}
	
	// Set information and read data for each room in this area.
	for (uint32_t i = 0; i < area.num_rooms; ++i) {
		
		// Set basic room information.
		area.rooms[i].id = (int)i;					// Unique numeric identifier for the room inside this area.
		area.rooms[i].size = area.room_size;		// Copy the area room size into the room struct.
		area.rooms[i].extent = area.room_extent;	// Copy the area room extent into the room struct.
		area.rooms[i].num_entity_spawners = 0;		// Feature not yet implemented.
		area.rooms[i].entity_spawners = nullptr;	// Feature not yet implemented.
		
		// Read room data.
		const int result = readRoomData2(pFile, &area.rooms[i]);
		if (result != 0) {
			logMsgF(LOG_LEVEL_ERROR, "Error creating area: error encountered while reading data for room %u (error code = %u).", i, result);
			free(area.rooms);
			area.rooms = nullptr;
			goto end_read;
		}

		// Map the 1D position of this room to an index into the room array inside the area struct.
		const int room_position = area_extent_index(area.extent, area.rooms[i].position); 
		area.positions_to_rooms[room_position] = i;
	}
	
end_read:
	return area;
}

static int readRoomData2(FILE *const pFile, Room *const pRoom) {
	
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