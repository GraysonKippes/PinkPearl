#ifndef ROOM_H
#define ROOM_H

#include <stdint.h>

typedef struct dimensions_t {
	uint32_t width;
	uint32_t length;
} dimensions_t;

// The size of a room, given as ratio of the default room size (24x15).
typedef enum room_size_t {
	ONE_TO_ONE,	// 24 x 15	-- Default room size, used in the Overworld.
	ONE_TO_THREE,	// 8 x 5	-- Tiny room size, used in small bonus areas.
	TWO_TO_THREE,	// 16 x 10	-- Small room size, used in some dungeons.
	FOUR_TO_THREE	// 32 x 20	-- Large room size, used in some dungeons.
} room_size_t;

#endif	// ROOM_H
