#include "renderer.h"

#include <stdint.h>

#include "model.h"
#include "vulkan/vulkan_manager.h"



const projection_bounds_t projection_bounds_8x5 = {
	.m_left = -4.0F, 	.m_right = 4.0F,
	.m_bottom = -2.5F, 	.m_top = 2.5F,
	.m_near = 15.0F,	.m_far = -15.0F
};

const projection_bounds_t projection_bounds_10x16 = {
	.m_left = -4.0F, 	.m_right = 4.0F,
	.m_bottom = -2.5F, 	.m_top = 2.5F,
	.m_near = 15.0F,	.m_far = -15.0F
};



static projection_bounds_t current_projection_bounds;

static const uint32_t num_room_model_slots = 2;

static uint32_t current_room_model_slot = num_room_model_slots - 1;



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

	room_model.m_num_vertices = 20;
	room_model.m_vertices = (float[20]){
		// Positions		Texture	
		left, top, depth,	0.0F, 0.0F,	// Top-left
		right, top, depth,	1.0F, 0.0F,	// Top-right
		right, bottom, depth,	1.0F, 1.0F,	// Bottom-right
		left, bottom, depth,	0.0F, 1.0F	// Bottom-left
	};

	room_model.m_num_indices = 6;
	room_model.m_indices = (index_t[6]){
		0, 1, 2,	// Triangle 1
		2, 3, 0		// Triangle 2
	};

	stage_model_data((render_handle_t)next_room_model_slot, room_model);

	current_room_model_slot = next_room_model_slot;

	projection_bounds_t room_projection_bounds = { 
		.m_left = left, 	.m_right = right,
		.m_bottom = -bottom, 	.m_top = -top,
		.m_near = 15.0F,	.m_far = -15.0F
	};

	update_projection_bounds(room_projection_bounds);
}
