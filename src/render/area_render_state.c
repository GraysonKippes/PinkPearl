#include "area_render_state.h"

#include "log/logging.h"
#include "util/allocate.h"
#include "util/time.h"

#include "math/lerp.h"
#include "vulkan/texture_manager.h"
#include "vulkan/vulkan_render.h"
#include "vulkan/compute/compute_area_mesh.h"

static area_render_state_t area_render_state = { 0 };

area_render_state_t get_global_area_render_state(void) {
	return area_render_state;
}

void area_render_state_reset(const area_t area, const room_t initial_room) {
	
	log_message(VERBOSE, "Resetting area render state...");
	
	destroy_texture_state(&area_render_state.tilemap_texture_state);
	area_render_state.tilemap_texture_state = make_new_texture_state(find_loaded_texture_handle("tilemap/dungeon4"));
	compute_area_mesh(area);
	
	// TODO - use reallocate function when it is implemented.
	deallocate((void **)&area_render_state.room_ids_to_cache_slots);
	area_render_state.num_room_ids = area.num_rooms;
	
	if (!allocate((void **)&area_render_state.room_ids_to_cache_slots, area_render_state.num_room_ids, sizeof(uint32_t))) {
		log_message(ERROR, "Error resetting area render state: failed to allocate room IDs to cache slots pointer-array.");
		return;
	}
	
	if (!allocate((void **)&area_render_state.room_ids_to_room_positions, area_render_state.num_room_ids, sizeof(offset_t))) {
		log_message(ERROR, "Error resetting area render state: failed to allocate room IDs to room positions pointer array.");
		deallocate((void **)&area_render_state.room_ids_to_cache_slots);
		return;
	}
	
	for (uint32_t i = 0; i < area_render_state.num_room_ids; ++i) {
		area_render_state.room_ids_to_cache_slots[i] = UINT32_MAX;
	}
	
	for (uint32_t i = 0; i < area_render_state.num_room_ids; ++i) {
		area_render_state.room_ids_to_room_positions[i] = area.rooms[i].position;
	}
	
	for (uint32_t i = 0; i < num_room_texture_cache_slots; ++i) {
		area_render_state.cache_slots_to_room_ids[i] = UINT32_MAX;
	}
	
	
	area_render_state.room_ids_to_cache_slots[initial_room.id] = 0;
	area_render_state.room_ids_to_room_positions[initial_room.id] = initial_room.position;
	area_render_state.cache_slots_to_room_ids[0] = (uint32_t)initial_room.id;
	
	area_render_state.area_extent = area.extent;
	area_render_state.room_size = area.room_size;
	
	area_render_state.current_cache_slot = 0;
	area_render_state.next_cache_slot = 0;
	
	upload_draw_data(area_render_state);
	create_room_texture(initial_room, area_render_state.current_cache_slot, area_render_state.tilemap_texture_state.handle);
	
	log_message(VERBOSE, "Done resetting area render state.");
}

bool area_render_state_is_scrolling(void) {
	return area_render_state.current_cache_slot != area_render_state.next_cache_slot;
}

bool area_render_state_next_room(const room_t next_room) {
	
	const uint32_t room_id = (uint32_t)next_room.id;
	
	if (room_id >= area_render_state.num_room_ids) {
		logf_message(ERROR, "Error setting area render state next room: given room ID (%u) is not less than number of room IDs (%u).", room_id, area_render_state.num_room_ids);
		return false;
	}
	
	uint32_t next_cache_slot = 0;
	
	// Check if the next room is already loaded.
	bool room_already_loaded = false;
	for (uint32_t i = 0; i < num_room_texture_cache_slots; ++i) {
		if (area_render_state.cache_slots_to_room_ids[i] == room_id) {
			room_already_loaded = true;
			next_cache_slot = i;
			break;
		}
	}
	
	// If the room is not already loaded, find the first cache slot not being used.
	if (!room_already_loaded) {
		for (uint32_t i = 0; i < num_room_texture_cache_slots; ++i) {
			if (i != area_render_state.current_cache_slot) {
				next_cache_slot = i;
			}
		}
	}
	
	area_render_state.next_cache_slot = next_cache_slot;
	area_render_state.scroll_start_time_ms = get_time_ms();
	
	area_render_state.room_ids_to_cache_slots[room_id] = area_render_state.next_cache_slot;
	area_render_state.cache_slots_to_room_ids[area_render_state.next_cache_slot] = room_id;
	
	// Update Vulkan engine.
	upload_draw_data(area_render_state);
	if (!room_already_loaded) {
		create_room_texture(next_room, area_render_state.next_cache_slot, area_render_state.tilemap_texture_state.handle);
	}
	
	return !room_already_loaded;
}

vector3F_t area_render_state_camera_position(void) {
	
	const extent_t room_extent = room_size_to_extent(area_render_state.room_size);
	
	const uint32_t current_room_id = area_render_state.cache_slots_to_room_ids[area_render_state.current_cache_slot];
	const offset_t current_room_position = area_render_state.room_ids_to_room_positions[current_room_id];
	const vector3F_t start = {
		.x = room_extent.width * current_room_position.x,
		.y = room_extent.length * current_room_position.y,
		.z = 0.0F
	};
	
	// If the area render state is not in scrolling state, just return the starting camera position.
	if (!area_render_state_is_scrolling()) {
		return start;
	}
	
	const uint32_t next_room_id = area_render_state.cache_slots_to_room_ids[area_render_state.next_cache_slot];
	const offset_t next_room_position = area_render_state.room_ids_to_room_positions[next_room_id];
	const vector3F_t end = {
		.x = room_extent.width * next_room_position.x,
		.y = room_extent.length * next_room_position.y,
		.z = 0.0F
	};
	
	static const uint64_t time_limit_ms = 1024;
	const uint64_t current_time_ms = get_time_ms();
	
	if (current_time_ms - area_render_state.scroll_start_time_ms >= time_limit_ms) {
		area_render_state.current_cache_slot = area_render_state.next_cache_slot;
		return end;
	}
	
	const double delta_time = (double)(current_time_ms - area_render_state.scroll_start_time_ms) / (double)(time_limit_ms);
	return vector3F_lerp(start, end, delta_time);
}

projection_bounds_t area_render_state_projection_bounds(void) {
	const extent_t room_extent = room_size_to_extent(area_render_state.room_size);
	const float top = -((float)room_extent.length / 2.0F);
	const float left = -((float)room_extent.width / 2.0F);
	const float bottom = -top;
	const float right = -left;
	return (projection_bounds_t){
		.left = left,
		.right = right,
		.bottom = bottom,
		.top = top,
		.near = 15.0F,
		.far = -15.0F
	};
}