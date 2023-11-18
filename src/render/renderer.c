#include "renderer.h"

#include <stdint.h>

#include "model.h"
#include "vulkan/texture_manager.h"
#include "vulkan/vulkan_manager.h"
#include "vulkan/vulkan_render.h"



const projection_bounds_t projection_bounds_8x5 = {
	.left = -4.0F, 	.right = 4.0F,
	.bottom = -2.5F, 	.top = 2.5F,
	.near = 15.0F,	.far = -15.0F
};

const projection_bounds_t projection_bounds_10x16 = {
	.left = -4.0F, 	.right = 4.0F,
	.bottom = -2.5F, 	.top = 2.5F,
	.near = 15.0F,	.far = -15.0F
};



static projection_bounds_t current_projection_bounds;

static const uint32_t num_room_model_slots = 2;

// Starts off at num_room_model_slots - 1 so that the first room starts at the next slot, which is 0.
static uint32_t current_room_model_slot = num_room_model_slots - 1;



void init_renderer(void) {
	create_vulkan_objects();
	create_vulkan_render_objects();
	load_textures();
}

void terminate_renderer(void) {
	destroy_textures();
	destroy_vulkan_render_objects();
	destroy_vulkan_objects();
}

void render_frame(double tick_delta) {
	draw_frame(tick_delta, current_projection_bounds);
}

void update_projection_bounds(projection_bounds_t new_projection_bounds) {
	current_projection_bounds = new_projection_bounds;
}

void upload_room_model(room_t room) {
	
	uint32_t next_room_model_slot = current_room_model_slot + 1;
	
	if (next_room_model_slot >= num_room_model_slots) {
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

	current_room_model_slot = next_room_model_slot;

	projection_bounds_t room_projection_bounds = { 
		.left = left, 	.right = right,
		.bottom = -bottom, 	.top = -top,
		.near = 15.0F,	.far = -15.0F
	};

	update_projection_bounds(room_projection_bounds);

	create_room_texture(room);
}
