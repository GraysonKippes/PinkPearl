#include "room.h"

#include <stdint.h>
#include <stdlib.h>

#include "log/logging.h"
#include "render/render_config.h"

const uint32_t num_room_sizes = NUM_ROOM_SIZES;

Extent room_size_to_extent(const RoomSize room_size) {
	switch (room_size) {
		case ROOM_SIZE_XS: return (Extent){ 8, 5 };
		case ROOM_SIZE_S: return (Extent){ 16, 10 };
		case ROOM_SIZE_M: return (Extent){ 24, 15 };
		case ROOM_SIZE_L: return (Extent){ 32, 20 };
	};
	logMsgF(LOG_LEVEL_ERROR, "Error converting room size to extent: invalid room size (%i).", (int)room_size);
	return (Extent){ 24, 15 };
}

void deleteRoom(Room room) {
	for (uint32_t i = 0; i < num_room_layers; ++i) {
		free(room.ppTileIndices[i]);
	}
	free(room.ppTileIndices);
	free(room.pWalls);
}