#include "room.h"

#include <stdint.h>
#include <stdlib.h>

#include "log/logging.h"

// Creates a room struct with the appropriate extent and array of tiles.
room_t create_room(room_size_t room_size) {
	
	room_t room = { 0 };

	room.size = room_size;
	
	switch (room_size) {
		case ONE_TO_THREE:
			room.extent.width = 8;
			room.extent.length = 5;
			break;
		case TWO_TO_THREE:
			room.extent.width = 16;
			room.extent.length = 10;
			break;
		case THREE_TO_THREE:
			room.extent.width = 24;
			room.extent.length = 15;
			break;
		case FOUR_TO_THREE:
			room.extent.width = 32;
			room.extent.length = 20;
			break;
	};

	uint64_t num_tiles = room.extent.width * room.extent.length;
	room.tiles = malloc(num_tiles * sizeof(tile_t));
	// Pointer checking is done outside of this function.

	return room;
}

extent_t room_size_to_extent(room_size_t room_size) {
	switch (room_size) {
		case ONE_TO_THREE: return (extent_t){ 8, 5 };
		case TWO_TO_THREE: return (extent_t){ 16, 10 };
		case THREE_TO_THREE: return (extent_t){ 24, 15 };
		case FOUR_TO_THREE: return (extent_t){ 32, 20 };
	};
}

int read_room_data(FILE *file, room_t *room_ptr) {
	
	// Must not close file.
	
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

	uint64_t num_tiles = room_ptr->extent.width * room_ptr->extent.length;
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

void destroy_room(room_t room) {
	free(room.tiles);
}
