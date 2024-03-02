#ifndef AREA_RENDER_STATE_H
#define AREA_RENDER_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "game/area/area.h"
#include "game/area/area_extent.h"
#include "game/area/room.h"
#include "util/offset.h"

#include "projection.h"
#include "render_config.h"
#include "texture_state.h"
#include "math/vector3F.h"

typedef struct area_render_state_t {
	
	area_extent_t area_extent;
	room_size_t room_size;
	
	// TODO - change to just the handle.
	texture_state_t tilemap_texture_state;
	// When the camera is not scrolling, current cache slot equals next cache slot.
	uint32_t current_cache_slot;
	uint32_t next_cache_slot;
	uint64_t scroll_start_time_ms;
	
	// Maps room IDs to room texture cache slots.
	uint32_t num_room_ids;	// Equal to number of rooms.
	uint32_t *room_ids_to_cache_slots;	// UINT32_MAX represents unmapped room ID.
	offset_t *room_ids_to_room_positions;
	
	// Maps room texture cache slots to room positions.
	// UINT32_MAX represents unmapped cache slot.
	uint32_t cache_slots_to_room_ids[NUM_ROOM_TEXTURE_CACHE_SLOTS];
	
} area_render_state_t;

// Returns a copy of the global area render state struct.
area_render_state_t get_global_area_render_state(void);

// Resets the global area render state to reflect the given area.
void area_render_state_reset(const area_t area, const room_t initial_room);

// Returns true if the current cache slot and the next cache slot are equal, false otherwise.
bool area_render_state_is_scrolling(void);

// Sets the next room to scroll to in the global area render state.
// Returns true if a new room texture needs to be generated, false otherwise.
bool area_render_state_next_room(const room_t next_room);

vector3F_t area_render_state_camera_position(void);
projection_bounds_t area_render_state_projection_bounds(void);

#endif	// AREA_RENDER_STATE_H