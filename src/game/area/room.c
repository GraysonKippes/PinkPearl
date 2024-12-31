#include "room.h"

#include <stdint.h>
#include <stdlib.h>
#include "log/Logger.h"
#include "render/render_config.h"
#include "util/allocate.h"

const uint32_t num_room_sizes = NUM_ROOM_SIZES;

Extent room_size_to_extent(const RoomSize room_size) {
	switch (room_size) {
		case ROOM_SIZE_XS: return (Extent){ 8, 5 };
		case ROOM_SIZE_S: return (Extent){ 16, 10 };
		case ROOM_SIZE_M: return (Extent){ 24, 15 };
		case ROOM_SIZE_L: return (Extent){ 32, 20 };
	};
	logMsg(loggerGame, LOG_LEVEL_ERROR, "Error converting room size to extent: invalid room size (%i).", (int)room_size);
	return (Extent){ 24, 15 };
}

void deleteRoom(Room room) {
	for (uint32_t i = 0; i < numRoomLayers; ++i) {
		heapFree(room.ppTileIndices[i]);
	}
	heapFree(room.ppTileIndices);
	heapFree(room.pWalls);
}