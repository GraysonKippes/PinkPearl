#include "fgm_file_parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log/logging.h"
#include "util/file_io.h"

#include "area_extent.h"

#define FGA_FILE_DIRECTORY (RESOURCE_PATH "data/area.fga")

static int read_room_data(FILE *file, room_t *room_ptr, const room_size_t room_size, const extent_t room_extent);

area_t parse_fga_file(const char *filename) {

	area_t area = { 0 };

	FILE *fga_file = fopen(FGA_FILE_DIRECTORY, "rb");
	if (fga_file == NULL) {
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

	// Read area extent.
	
	if (!read_data(fga_file, 4, 1, &area.extent.x1)) {
		log_message(ERROR, "Error reading area file: failed to read area extent coordinate x1.");
		goto end_read;
	}

	if (!read_data(fga_file, 4, 1, &area.extent.y1)) {
		log_message(ERROR, "Error reading area file: failed to read area extent coordinate y1.");
		goto end_read;
	}

	if (!read_data(fga_file, 4, 1, &area.extent.x2)) {
		log_message(ERROR, "Error reading area file: failed to read area extent coordinate x2.");
		goto end_read;
	}

	if (!read_data(fga_file, 4, 1, &area.extent.y2)) {
		log_message(ERROR, "Error reading area file: failed to read area extent coordinate y2.");
		goto end_read;
	}

	const int area_width = area_extent_width(area.extent);
	const int area_length = area_extent_length(area.extent);
	const long int extent_area = area_width * area_length;

	if (area_width < 1) {
		logf_message(ERROR, "Error creating area: the width of the area is not positive (%i).", area_width);
		goto end_read;
	}

	if (area_length < 1) {
		logf_message(ERROR, "Error creating area: the length of the area is not positive (%i).", area_length);
		goto end_read;
	}

	// Read room extent type.
	
	uint32_t room_extent_type = 0;
	if (!read_data(fga_file, 4, 1, &room_extent_type)) {
		log_message(ERROR, "Error reading area file: failed to read room extent type.");
		goto end_read;
	}

	// TODO - this is a hotfix, adjust the enum values.
	room_extent_type--;
	if (room_extent_type > 3) {
		logf_message(ERROR, "Error creating area: room extent type is invalid value (%u).", room_extent_type);
	}

	const room_size_t room_size = (room_size_t)room_extent_type;
	area.room_extent = room_size_to_extent(room_size);

	// Read number of rooms.
	
	if (!read_data(fga_file, 4, 1, &area.num_rooms)) {
		log_message(ERROR, "Error reading area file: failed to read number of rooms.");
		goto end_read;
	}

	if ((int64_t)area.num_rooms > extent_area) {
		logf_message(ERROR, "Error reading area fga_file: `area.num_rooms` (%u) is greater than `area.extent.width` times `area.extent.height` (%lu).", area.num_rooms, extent_area);
		goto end_read;
	}

	area.rooms = calloc(area.num_rooms, sizeof(room_t));
	if (area.rooms == NULL) {
		log_message(ERROR, "Error creating area: allocation of area.rooms failed.");
		goto end_read;
	}

	area.positions_to_rooms = calloc(extent_area, sizeof(int));
	if (area.positions_to_rooms == NULL) {
		log_message(ERROR, "Error creating area: allocation of area.positions_to_rooms failed.");
		free(area.rooms);
		area.rooms = NULL;
		goto end_read;
	}

	for (size_t i = 0; i < (size_t)extent_area; ++i) {
		area.positions_to_rooms[i] = -1;
	}

	for (uint32_t i = 0; i < area.num_rooms; ++i) {

		area.rooms[i].id = (int)i;

		uint32_t room_position = 0;
		if (!read_data(fga_file, 4, 1, &room_position)) {
			log_message(ERROR, "Error reading area file: failed to read room position.");
			free(area.rooms);
			area.rooms = NULL;
			goto end_read;
		}

		if ((int64_t)room_position >= extent_area) {
			logf_message(WARNING, "Warning creating area: room position (%u) is greater than extent area (%li).", room_position, extent_area);
		}

		area.positions_to_rooms[room_position] = i;

		int result = read_room_data(fga_file, (area.rooms + i), room_size, area.room_extent);
		if (result != 0) {
			logf_message(ERROR, "Error creating area: error encountered while reading data for room %u (Error code = %u).", i, result);
			free(area.rooms);
			area.rooms = NULL;
			goto end_read;
		}

		area.rooms[i].position.x = ((int64_t)room_position % area_width) + area.extent.x1;
		area.rooms[i].position.y = ((int64_t)room_position / area_width) + area.extent.y1;
	}

end_read:
	fclose(fga_file);
	return area;
}

static int read_room_data(FILE *file, room_t *room_ptr, const room_size_t room_size, const extent_t room_extent) {
	
	// Must not close file.
	
	room_ptr->size = room_size;
	room_ptr->extent = room_extent;

	room_ptr->tiles = calloc(extent_area(room_extent), sizeof(tile_t));
	if (room_ptr->tiles == NULL) {
		log_message(ERROR, "Error creating room: failed to allocate tile array.");
		return -1;
	}

	// Read wall information.

	uint32_t num_walls = 0;
	fread(&num_walls, 4, 1, file);

	if (num_walls > 0) {

		room_ptr->num_walls = num_walls;
		room_ptr->walls = calloc(num_walls, sizeof(rect_t));
		if (room_ptr->walls == NULL) {
			log_message(ERROR, "Error creating room: failed to allocate wall array.");
			return -2;
		}

		for (uint32_t i = 0; i < num_walls; ++i) {
			rect_t wall = { 0 };
			fread(&wall.x1, sizeof(double), 1, file);
			fread(&wall.y1, sizeof(double), 1, file);
			fread(&wall.x2, sizeof(double), 1, file);
			fread(&wall.y2, sizeof(double), 1, file);
			room_ptr->walls[i] = wall;
		}
	}

	// Read tile information.

	const uint64_t num_tiles = extent_area(room_ptr->extent);
	uint64_t tiles_filled = 0;

	// Read data for each tile here.
	while (tiles_filled < num_tiles) {

		uint32_t fill_range = 0;
		tile_t tile = { 0 };

		if (!read_data(file, 4, 1, &fill_range)) {
			log_message(ERROR, "Error reading room data: failed to read tile fill range.");
			return -3;
		}

		if (!read_data(file, 4, 1, &tile.tilemap_slot)) {
			log_message(ERROR, "Error reading room data: failed to read tile tilemap slot.");
			return -4;
		}

		if (fill_range == 0) {
			log_message(ERROR, "Error creating room: tile fill range is zero.");
			free(room_ptr->tiles);
			return -5;
		}

		if (tiles_filled + (uint64_t)fill_range > num_tiles) {
			logf_message(ERROR, "Error creating room: tile fill range (%lu + %u) exceeds room area (%lu).", tiles_filled, fill_range, num_tiles);
			free(room_ptr->tiles);
			return -6;
		}

		for (uint32_t j = 0; j < fill_range; ++j) {
			room_ptr->tiles[tiles_filled + (uint64_t)j] = tile;
		}
		tiles_filled += (uint64_t)fill_range;
	}

	return 0;
}
