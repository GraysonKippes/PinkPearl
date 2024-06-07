#include "fgm_file_parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log/logging.h"
#include "render/render_config.h"
#include "util/file_io.h"

#include "area_extent.h"

#define FGA_FILE_DIRECTORY (RESOURCE_PATH "data/area2.fga")

static int readRoomData(FILE *file, Room *const pRoom);
static void transposeTileData(const Extent extent, const Tile *const pTiles, uint16_t **ppTileIndices);

area_t parse_fga_file(const char *filename) {

	area_t area = { 0 };

	FILE *fga_file = fopen(FGA_FILE_DIRECTORY, "rb");
	if (fga_file == nullptr) {
		log_message(ERROR, "Error reading area file: failed to open file.");
		return area;
	}

	// Check file label.
	char label[4];
	fread(label, 1, 4, fga_file);
	if (strcmp(label, "FGA") != 0) {
		logf_message(ERROR, "Error reading area file: invalid file format; found label \"%s\".", label);
		goto end_read;
	}
	fseek(fga_file, 0x10, SEEK_SET);

	// Read area extent.
	
	if (!read_data(fga_file, sizeof(area.extent), 1, &area.extent)) {
		log_message(ERROR, "Error reading area file: failed to read area extent.");
		goto end_read;
	}

	const int area_width = area_extent_width(area.extent);
	const int area_length = area_extent_length(area.extent);
	const long int extentArea = area_width * area_length;

	if (area_width < 1) {
		logf_message(ERROR, "Error creating area: the width of the area is not positive (%i).", area_width);
		goto end_read;
	}

	if (area_length < 1) {
		logf_message(ERROR, "Error creating area: the length of the area is not positive (%i).", area_length);
		goto end_read;
	}

	fseek(fga_file, 0x20, SEEK_SET);

	// Read room extent type.
	
	uint32_t room_extent_type = 0;
	if (!read_data(fga_file, 4, 1, &room_extent_type)) {
		log_message(ERROR, "Error reading area file: failed to read room extent type.");
		goto end_read;
	}

	if (room_extent_type >= num_room_sizes) {
		logf_message(ERROR, "Error creating area: room extent type is invalid value (%u).", room_extent_type);
		goto end_read;
	}

	const RoomSize room_size = (RoomSize)room_extent_type;
	area.room_size = room_size;
	area.room_extent = room_size_to_extent(room_size);

	// Read number of rooms.
	
	if (!read_data(fga_file, 4, 1, &area.num_rooms)) {
		log_message(ERROR, "Error reading area file: failed to read number of rooms.");
		goto end_read;
	}

	if ((int64_t)area.num_rooms > extentArea) {
		logf_message(ERROR, "Error reading area fga_file: `area.num_rooms` (%u) is greater than `area.extent.width` times `area.extent.height` (%lu).", area.num_rooms, extentArea);
		goto end_read;
	}

	area.rooms = calloc(area.num_rooms, sizeof(Room));
	if (area.rooms == nullptr) {
		log_message(ERROR, "Error creating area: allocation of area.rooms failed.");
		goto end_read;
	}

	area.positions_to_rooms = calloc(extentArea, sizeof(int));
	if (area.positions_to_rooms == nullptr) {
		log_message(ERROR, "Error creating area: allocation of area.positions_to_rooms failed.");
		free(area.rooms);
		area.rooms = nullptr;
		goto end_read;
	}

	for (size_t i = 0; i < (size_t)extentArea; ++i) {
		area.positions_to_rooms[i] = -1;
	}

	fseek(fga_file, 0x30, SEEK_SET);
	for (uint32_t i = 0; i < area.num_rooms; ++i) {

		area.rooms[i].id = (int)i;
		area.rooms[i].size = area.room_size;
		area.rooms[i].extent = area.room_extent;
		area.rooms[i].num_entity_spawners = 0;
		area.rooms[i].entity_spawners = nullptr;

		const int result = readRoomData(fga_file, &area.rooms[i]);
		if (result != 0) {
			logf_message(ERROR, "Error creating area: error encountered while reading data for room %u (Error code = %u).", i, result);
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

	if (!read_data(file, sizeof(offset_t), 1, &pRoom->position)) {
		log_message(ERROR, "Error reading area file: failed to read room position.");
		return -1;
	}
	
	pRoom->ppTileIndices = calloc(num_room_layers, sizeof(uint16_t *));
	for (uint32_t i = 0; i < num_room_layers; ++i) {
		pRoom->ppTileIndices[i] = calloc(num_tiles, sizeof(uint16_t));
	}
	
	Tile *pTiles = calloc(num_tiles, sizeof(Tile));
	if (!pTiles) {
		log_message(ERROR, "Error reading area file: failed to allocate tile array.");
		return -2;
	}

	// Read wall information.
	uint32_t num_walls = 0;
	fread(&num_walls, 4, 1, file);
	fseek(file, 4, SEEK_CUR);
	if (num_walls > 0) {

		pRoom->num_walls = num_walls;
		pRoom->walls = calloc(num_walls, sizeof(rect_t));
		if (pRoom->walls == nullptr) {
			log_message(ERROR, "Error reading area file: failed to allocate wall array.");
			return -3;
		}

		if (!read_data(file, sizeof(rect_t), pRoom->num_walls, pRoom->walls)) {
			log_message(ERROR, "Error reading area file: failed to read room wall data.");
			return -4;
		}
	}

	// Read tile information.
	if (!read_data(file, sizeof(Tile), num_tiles, pTiles)) {
		log_message(ERROR, "Error reading area file: failed to read room tile data.");
		return -5;
	}
	transposeTileData(pRoom->extent, pTiles, pRoom->ppTileIndices);

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