#include "room.h"

#include <stdint.h>
#include <stdlib.h>

#include "log/logging.h"
#include "render/render_config.h"

const uint32_t num_room_sizes = NUM_ROOM_SIZES;

Extent room_size_to_extent(const RoomSize room_size) {
	switch (room_size) {
		case ONE_TO_THREE: return (Extent){ 8, 5 };
		case TWO_TO_THREE: return (Extent){ 16, 10 };
		case THREE_TO_THREE: return (Extent){ 24, 15 };
		case FOUR_TO_THREE: return (Extent){ 32, 20 };
	};
	logf_message(ERROR, "Error converting room size to extent: invalid room size (%i).", (int)room_size);
	return (Extent){ 24, 15 };
}

void deleteRoom(Room room) {
	for (uint32_t i = 0; i < num_room_layers; ++i) {
		free(room.ppTileIndices[i]);
	}
	free(room.ppTileIndices);
	free(room.walls);
}