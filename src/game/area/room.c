#include "room.h"

#include <stdint.h>
#include <stdlib.h>

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
