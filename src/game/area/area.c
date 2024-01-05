#include "area.h"

#include <stddef.h>
#include <stdint.h>

#include "log/logging.h"

room_t *area_get_room_ptr(const area_t area, const offset_t map_position) {

	if (map_position.x < area.extent.x1 || map_position.x > area.extent.x2 
			|| map_position.y < area.extent.y1 || map_position.y > area.extent.y2) {
		logf_message(WARNING, "Warning getting room pointer: map position (%i, %i) of room falls outside of the area.", map_position.x, map_position.y);
		return NULL;
	}

	const int32_t area_width = area.extent.x2 - area.extent.x1;
	if (area_width < 1) {
		logf_message(WARNING, "Warning getting room pointer: the width of the area is not positive (%i).", area_width);
		return NULL;
	}

	const int32_t position_index = map_position.y * area_width + map_position.x;
	const int32_t room_index = area.positions_to_rooms[position_index];
	if (room_index >= area.num_rooms) {
		logf_message(WARNING, "Warning getting room pointer: room index (%i) is greater than or equal to number of rooms.", room_index);
		return NULL;
	}

	return area.rooms + room_index;
}
