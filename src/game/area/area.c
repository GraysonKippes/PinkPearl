#include "area.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log/logging.h"

#define FGA_FILE_DIRECTORY (RESOURCE_PATH "data/area.fga")

void read_area_data(const char *path, area_t *area_ptr) {

	log_message(VERBOSE, "Reading area file...");

	if (area_ptr == NULL) {
		log_message(ERROR, "Error creating area: pointer to area is null.");
		return;
	}

	FILE *file = fopen(FGA_FILE_DIRECTORY, "rb");
	if (file == NULL) {
		logf_message(ERROR, "Error reading area file: could not open area file at \"%s\".", path);
		return;
	}

	// Check file label.
	char label[4];
	fread(label, 1, 4, file);
	if (strcmp(label, "FGA") != 0) {
		logf_message(ERROR, "Error reading area file: invalid file format; found label \"%s\".", label);
		fclose(file);
		return;
	}

	// TODO - figure out all the endianness shit

	fread(&area_ptr->extent.width, 4, 1, file);
	fread(&area_ptr->extent.length, 4, 1, file);
	fread(&area_ptr->num_rooms, 4, 1, file);

	uint64_t extent_area = area_ptr->extent.width * area_ptr->extent.length;

	logf_message(WARNING, "Area data: width = %u, length = %u, num. rooms = %u", 
			area_ptr->extent.width, area_ptr->extent.length, area_ptr->num_rooms);

	if (area_ptr->num_rooms > extent_area) {
		logf_message(ERROR, "Error reading area file: `area.num_rooms` (%u) is greater than `area.extent.width` times `area.extent.height` (%lu).", area_ptr->num_rooms, extent_area);
		fclose(file);
		return;
	}

	area_ptr->rooms = malloc(area_ptr->num_rooms * sizeof(room_t));
	if (area_ptr->rooms == NULL) {
		log_message(ERROR, "Error creating area: failed to allocate room array.");
		fclose(file);
		return;
	}

	area_ptr->positions_to_rooms = malloc(area_ptr->num_rooms * sizeof(uint32_t));
	if (area_ptr->positions_to_rooms == NULL) {
		log_message(ERROR, "Error creating area: failed to allocate positions-to-rooms array.");
		free(area_ptr->rooms);
		fclose(file);
		return;
	}

	for (uint32_t i = 0; i < area_ptr->num_rooms; ++i) {

		logf_message(WARNING, "Reading data for room %u.", i);

		uint32_t room_position = 0;
		fread(&room_position, 4, 1, file);
		area_ptr->positions_to_rooms[i] = room_position;

		int result = read_room_data(file, (area_ptr->rooms + i));
		if (result != 0) {
			logf_message(ERROR, "Error creating area: error encountered while reading data for room %u (Error code = %u).", i, result);
			free(area_ptr->rooms);
			area_ptr->rooms = NULL;
			fclose(file);
			return;
		}
	}

	fclose(file);
}

room_t *area_get_room_ptr(area_t area, offset_t map_position) {
	
	if (map_position.x >= area.extent.width || map_position.y >= area.extent.length) {
		return NULL;
	}
	
	const uint32_t position_index = map_position.y * area.extent.width + map_position.x;
	const uint32_t room_index = area.positions_to_rooms[position_index];

	if (room_index >= area.num_rooms) {
		return NULL;
	}

	return area.rooms + room_index;
}
