#include "renderer.h"

#include <GLFW/glfw3.h>

#include "config.h"
#include "log/logging.h"

#include "render_config.h"
#include "render_object.h"
#include "math/vector3F.h"
#include "vulkan/texture_manager.h"
#include "vulkan/vulkan_manager.h"
#include "vulkan/vulkan_render.h"



#define DATA_PATH (RESOURCE_PATH "data/")
#define FGT_PATH (RESOURCE_PATH "data/textures.fgt")



static projection_bounds_t current_projection_bounds;

// Starts off at num_room_render_object_slots - 1 so that the next slot--which the room starts at--is 0.
static uint32_t current_room_render_object_slot = NUM_ROOM_RENDER_OBJECT_SLOTS - 1;



void init_renderer(void) {

	create_vulkan_objects();
	create_vulkan_render_objects();
	load_premade_models();

	// Load textures
	texture_pack_t texture_pack = parse_fgt_file(FGT_PATH);
	load_textures(texture_pack);
	destroy_texture_pack(&texture_pack);

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {

		if (i >= num_room_render_object_slots) {
			set_model_texture(i, get_loaded_texture(0));
		}

		reset_render_position((render_object_positions + i), zero_vector3F);
	}

	upload_model(2, get_premade_model(0), find_loaded_texture("entity/pearl"));

	enable_render_object_slot(0);
	enable_render_object_slot(2);

	get_model_texture_ptr(2)->current_animation_cycle = 3;
}

void terminate_renderer(void) {
	destroy_vulkan_render_objects();
	destroy_vulkan_objects();
	free_premade_models();
}

void render_frame(float tick_delta_time) {
	glfwPollEvents();
	draw_frame(tick_delta_time, current_projection_bounds);
}

void update_projection_bounds(projection_bounds_t new_projection_bounds) {
	current_projection_bounds = new_projection_bounds;
}

void upload_model(uint32_t slot, model_t model, texture_t texture) {

	if (slot >= num_render_object_slots) {
		logf_message(ERROR, "Error uploading model: render object slot (%u) exceeds total number of render object slots (%u).", slot, num_render_object_slots);
		return;
	}

	if (slot < num_room_render_object_slots) {
		logf_message(FATAL, "Error uploading model: attempted uploading non-room model in a render object slot (%u) reserved for room models.", slot);
		return;
	}

	stage_model_data(slot, model);
	set_model_texture(slot, texture);
}

void upload_room_model(room_t room) {
	
	uint32_t next_room_model_slot = current_room_render_object_slot + 1;
	
	if (next_room_model_slot >= num_room_render_object_slots) {
		next_room_model_slot = 0;
	}

	const float top = -((float)room.extent.length / 2.0F);
	const float left = -((float)room.extent.width / 2.0F);
	const float bottom = -top;
	const float right = -left;

	static const float depth = 0.0F;

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

	stage_model_data((render_handle_t)next_room_model_slot, room_model);

	projection_bounds_t room_projection_bounds = { 
		.left = left, 	.right = right,
		.bottom = -bottom, 	.top = -top,
		.near = 15.0F,	.far = -15.0F
	};

	update_projection_bounds(room_projection_bounds);

	create_room_texture(room, next_room_model_slot);

	current_room_render_object_slot = next_room_model_slot;
}
