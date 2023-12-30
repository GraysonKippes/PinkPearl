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
		default:
		case THREE_TO_THREE: return (extent_t){ 24, 15 };
		case FOUR_TO_THREE: return (extent_t){ 32, 20 };
	};
}

void destroy_room(room_t room) {
	free(room.tiles);
}
