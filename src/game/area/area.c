#include "area.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log/logging.h"

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
