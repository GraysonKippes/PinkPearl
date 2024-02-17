#include "renderer.h"

#include <stdbool.h>

#include <GLFW/glfw3.h>

#include "config.h"
#include "log/logging.h"
#include "util/time.h"

#include "render_config.h"
#include "render_object.h"
#include "math/vector3F.h"
#include "vulkan/texture_manager.h"
#include "vulkan/vulkan_manager.h"
#include "vulkan/vulkan_render.h"



#define DATA_PATH (RESOURCE_PATH "data/")
#define FGT_PATH (RESOURCE_PATH "data/textures.fgt")



static vector3F_t camera_position = { 0 };
static vector3F_t old_camera_position = { 0 };
static vector3F_t new_camera_position = { 0 };
static bool camera_scrolling = false;
static uint64_t time_ms_since_scroll_start = 0;

static projection_bounds_t current_projection_bounds;

// Starts off at num_room_render_object_slots - 1 so that the next slot--which the room starts at--is 0.
// The preprocessor constant is used for static initialization.
static uint32_t current_room_cache_slot = NUM_ROOM_RENDER_OBJECT_SLOTS - 1;

// Keeps track of which rooms are already loaded into the renderer.
static int loaded_room_ids[NUM_ROOM_RENDER_OBJECT_SLOTS];

static texture_state_t tilemap_texture_state;

static void scroll_camera(void);



void init_renderer(void) {

	create_vulkan_objects();
	create_vulkan_render_objects();
	load_premade_models();

	// Load textures
	texture_pack_t texture_pack = parse_fgt_file(FGT_PATH);
	load_textures(texture_pack);
	destroy_texture_pack(&texture_pack);
	set_tilemap("tilemap/dungeon4");

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		reset_render_position(&render_object_positions[i], zero_vector3F);
		render_object_texture_states[i] = missing_texture_state();
	}

	for (uint32_t i = 0; i < num_room_render_object_slots; ++i) {
		loaded_room_ids[i] = -1;
	}

	enable_render_object_slot(0);
	enable_render_object_slot(2);
}

void terminate_renderer(void) {
	destroy_vulkan_render_objects();
	destroy_vulkan_objects();
	free_premade_models();
}

void render_frame(float tick_delta_time) {

	if (camera_scrolling) {
		scroll_camera();
	}

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		texture_state_animate(render_object_texture_states + i);
	}

	glfwPollEvents();
	draw_frame(tick_delta_time, camera_position, current_projection_bounds);
}

void settle_render_positions(void) {
	
	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		settle_render_position(render_object_positions + i);
	}
}

void update_projection_bounds(projection_bounds_t new_projection_bounds) {
	current_projection_bounds = new_projection_bounds;
}

void set_tilemap(const char *const tilemap_name) {
	destroy_texture_state(&tilemap_texture_state);
	tilemap_texture_state = make_new_texture_state(find_loaded_texture_handle(tilemap_name));
}

void upload_model(uint32_t slot, model_t model, const char *const texture_name) {

	if (slot >= num_render_object_slots) {
		logf_message(ERROR, "Error uploading model: render object slot (%u) exceeds total number of render object slots (%u).", slot, num_render_object_slots);
		return;
	}

	if (slot < num_room_render_object_slots) {
		logf_message(ERROR, "Error uploading model: attempted uploading non-room model in a render object slot (%u) reserved for room models.", slot);
		return;
	}

	stage_model_data(slot, model);
	const texture_state_t texture_state = make_new_texture_state(find_loaded_texture_handle(texture_name));
	swap_render_object_texture_state(slot, texture_state);
}

void upload_room_model(const room_t room) {
	
	if (++current_room_cache_slot >= room_texture_cache_size) {
		current_room_cache_slot = 0;
	}

	if (room.id == loaded_room_ids[current_room_cache_slot]) {
		enable_render_object_slot(current_room_cache_slot);
		return;
	}

	const float top = -((float)room.extent.length / 2.0F);
	const float left = -((float)room.extent.width / 2.0F);
	const float bottom = -top;
	const float right = -left;
	static const float depth = 0.0F;

	// TODO - generate room/area meshes with mesh shader.
	// The model array data can be allocated on the stack instead of the heap because it will be fully uploaded to a GPU buffer before this function returns.
	model_t room_model = { 0 };
	room_model.num_vertices = 20;
	room_model.vertices = (float[20]){
		// Positions		Texture	
		left, top, depth,	0.0F, 0.0F,	// Top-left
		right, top, depth,	1.0F, 0.0F,	// Top-right
		right, bottom, depth,	1.0F, 1.0F,	// Bottom-right
		left, bottom, depth,	0.0F, 1.0F	// Bottom-left
	};
	room_model.num_indices = 6;
	room_model.indices = (index_t[6]){
		0, 1, 2,	// Triangle 1
		2, 3, 0		// Triangle 2
	};
	stage_model_data((render_handle_t)current_room_cache_slot, room_model);

	const projection_bounds_t room_projection_bounds = { 
		.left = left, 	.right = right,
		.bottom = -bottom, 	.top = -top,
		.near = 15.0F,	.far = -15.0F
	};
	update_projection_bounds(room_projection_bounds);

	create_room_texture(room, current_room_cache_slot, tilemap_texture_state.handle);
	const texture_state_t room_texture_state = make_new_texture_state(get_room_texture_handle(room.size, current_room_cache_slot));
	swap_render_object_texture_state((render_handle_t)current_room_cache_slot, room_texture_state);

	const vector3F_t room_model_position = {
		.x = (float)room.extent.width * (float)room.position.x,
		.y = (float)room.extent.length * (float)room.position.y,
		.z = depth
	};
	reset_render_position(render_object_positions + current_room_cache_slot, room_model_position);

	loaded_room_ids[current_room_cache_slot] = room.id;
	enable_render_object_slot(current_room_cache_slot);
}

void begin_room_scroll(const extent_t room_extent, const offset_t previous_room_position, const offset_t next_room_position) {

	old_camera_position = camera_position;
	
	const offset_t room_travel_vector = offset_subtract(next_room_position, previous_room_position);
	const vector3F_t room_scroll_vector = {
		.x = (float)room_travel_vector.x * (float)room_extent.width,
		.y = (float)room_travel_vector.y * (float)room_extent.length,
		.z = old_camera_position.z
	};
	new_camera_position = vector3F_add(new_camera_position, room_scroll_vector);

	time_ms_since_scroll_start = get_time_ms();
	camera_scrolling = true;
}

bool is_camera_scrolling(void) {
	return camera_scrolling;
}

static void scroll_camera(void) {

	static const uint64_t scroll_time_limit_ms = 1024; // Scrolling should last 3 seconds.
	const uint64_t scroll_delta_time_ms = get_time_ms() - time_ms_since_scroll_start;

	if (scroll_delta_time_ms < scroll_time_limit_ms) {

		const float normalized_delta_time = (float)scroll_delta_time_ms / (float)scroll_time_limit_ms;

		const vector3F_t camera_position_step = {
			.x = normalized_delta_time * (new_camera_position.x - old_camera_position.x),
			.y = normalized_delta_time * (new_camera_position.y - old_camera_position.y)
		};
		camera_position = vector3F_add(old_camera_position, camera_position_step);
	}
	else {
		camera_position = new_camera_position;
		old_camera_position = camera_position;
		camera_scrolling = false;
	}
}
