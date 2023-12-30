#include "fgm_file_parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log/logging.h"
#include "util/file_io.h"

#define FGA_FILE_DIRECTORY (RESOURCE_PATH "data/area.fga")

int read_room_data(FILE *file, room_t *room_ptr);

area_t parse_fga_file(const char *filename) {

	area_t area = { 0 };

	FILE *fga_file = fopen(FGA_FILE_DIRECTORY, "rb");
	if (fga_file == NULL) {
		return area;
	}

	// Check file label.
	char label[4];
	fread(label, 1, 4, fga_file);
	if (strcmp(label, "FGA") != 0) {
		logf_message(ERROR, "Error reading area file: invalid file format; found label \"%s\".", label);
		goto end_read;
	}

	// Read basic area info.
	fread(&area.extent.width, 4, 1, fga_file);
	fread(&area.extent.length, 4, 1, fga_file);
	fread(&area.num_rooms, 4, 1, fga_file);

	uint64_t extent_area = area.extent.width * area.extent.length;

	logf_message(WARNING, "Area data: width = %u, length = %u, num. rooms = %u", 
			area.extent.width, area.extent.length, area.num_rooms);

	if (area.num_rooms > extent_area) {
		logf_message(ERROR, "Error reading area fga_file: `area.num_rooms` (%u) is greater than `area.extent.width` times `area.extent.height` (%lu).", area.num_rooms, extent_area);
		goto end_read;
	}

	area.rooms = calloc(area.num_rooms, sizeof(room_t));
	if (area.rooms == NULL) {
		log_message(ERROR, "Error creating area: failed to allocate room array.");
		goto end_read;
	}

	area.positions_to_rooms = malloc(area.num_rooms * sizeof(uint32_t));
	if (area.positions_to_rooms == NULL) {
		log_message(ERROR, "Error creating area: failed to allocate positions-to-rooms array.");
		free(area.rooms);
		goto end_read;
	}

	for (uint32_t i = 0; i < area.num_rooms; ++i) {

		logf_message(WARNING, "Reading data for room %u.", i);

		uint32_t room_position = 0;
		fread(&room_position, 4, 1, fga_file);
		area.positions_to_rooms[i] = room_position;

		int result = read_room_data(fga_file, (area.rooms + i));
		if (result != 0) {
			logf_message(ERROR, "Error creating area: error encountered while reading data for room %u (Error code = %u).", i, result);
			free(area.rooms);
			area.rooms = NULL;
			goto end_read;
		}
	}

end_read:
	fclose(fga_file);
	return area;
}

int read_room_data(FILE *file, room_t *room_ptr) {
	
	// Must not close file.
	
	// Read room size information.

	uint32_t room_size = 0;
	fread(&room_size, 4, 1, file);

	if (room_size > 3) {
		logf_message(ERROR, "Error reading room data: invalid room size code (%u).", room_size);
		return 1;
	}

	*room_ptr = create_room((room_size_t)room_size);
	if (room_ptr->tiles == NULL) {
		log_message(ERROR, "Error creating room: failed to allocate tile array.");
		return 2;
	}

	// Read wall information.

	uint32_t num_walls = 0;
	fread(&num_walls, 4, 1, file);

	logf_message(WARNING, "num_walls = %u", num_walls);

	if (num_walls > 0) {

		room_ptr->num_walls = num_walls;
		room_ptr->walls = calloc(num_walls, sizeof(rect_t));
		if (room_ptr->walls == NULL) {
			log_message(ERROR, "Error creating room: failed to allocate wall array.");
			return 3;
		}

		for (uint32_t i = 0; i < num_walls; ++i) {
			rect_t wall = { 0 };
			fread(&wall.x1, sizeof(double), 1, file);
			fread(&wall.y1, sizeof(double), 1, file);
			fread(&wall.x2, sizeof(double), 1, file);
			fread(&wall.y2, sizeof(double), 1, file);
			logf_message(WARNING, "Wall %u: (%.1f, %.1f; %.1f, %.1f)", i, wall.x1, wall.y1, wall.x2, wall.y2);
			room_ptr->walls[i] = wall;
			//fread((room_ptr->walls + i), 4, 4, file);
		}
	}

	// Read tile information.

	const uint64_t num_tiles = room_ptr->extent.width * room_ptr->extent.length;
	uint64_t tiles_filled = 0;

	logf_message(WARNING, "num_tiles = %lu", num_tiles);

	// Read data for each tile here.
	while (tiles_filled < num_tiles) {

		uint32_t fill_range = 0;
		tile_t tile = { 0 };

		fread(&fill_range, 4, 1, file);
		fread(&tile, 4, 1, file);

		logf_message(WARNING, "fill_range = %u", fill_range);

		if (fill_range == 0) {
			log_message(ERROR, "Error creating room: tile fill range is zero.");
			free(room_ptr->tiles);
			return 3;
		}

		if (tiles_filled + (uint64_t)fill_range > num_tiles) {
			logf_message(ERROR, "Error creating room: tile fill range (%lu + %u) exceeds room area (%lu).", tiles_filled, fill_range, num_tiles);
			free(room_ptr->tiles);
			return 4;
		}

		for (uint32_t j = 0; j < fill_range; ++j) {
			room_ptr->tiles[tiles_filled + (uint64_t)j] = tile;
		}

		tiles_filled += (uint64_t)fill_range;
		logf_message(WARNING, "tiles_filled = %lu", tiles_filled);
	}

	return 0;
}
