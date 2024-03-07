#include "room.h"

#include <stdint.h>
#include <stdlib.h>

#include "log/logging.h"

const uint32_t num_room_sizes = NUM_ROOM_SIZES;

extent_t room_size_to_extent(const room_size_t room_size) {
	switch (room_size) {
		case ONE_TO_THREE: return (extent_t){ 8, 5 };
		case TWO_TO_THREE: return (extent_t){ 16, 10 };
		case THREE_TO_THREE: return (extent_t){ 24, 15 };
		case FOUR_TO_THREE: return (extent_t){ 32, 20 };
	};
	logf_message(ERROR, "Error converting room size to extent: invalid room size (%i).", (int)room_size);
	return (extent_t){ 24, 15 };
}

void destroy_room(room_t room) {
	free(room.walls);
	free(room.tiles);
}
