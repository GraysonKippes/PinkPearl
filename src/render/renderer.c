#include "renderer.h"

#include <stdbool.h>

#include <GLFW/glfw3.h>

#include "config.h"
#include "log/log_stack.h"
#include "log/logging.h"
#include "util/time.h"

#include "area_render_state.h"
#include "render_config.h"
#include "render_object.h"
#include "math/lerp.h"
#include "math/vector3F.h"
#include "vulkan/texture_manager.h"
#include "vulkan/vulkan_manager.h"
#include "vulkan/vulkan_render.h"

#define DATA_PATH (RESOURCE_PATH "data/")
#define FGT_PATH (RESOURCE_PATH "data/textures.fgt")

void init_renderer(void) {

	log_stack_push("Renderer");

	create_vulkan_objects();
	create_vulkan_render_objects();
	load_premade_models();

	// Load textures
	texture_pack_t texture_pack = parse_fgt_file(FGT_PATH);
	load_textures(texture_pack);
	destroy_texture_pack(&texture_pack);

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		reset_render_position(&render_object_positions[i], zero_vector3F);
		render_object_texture_states[i] = missing_texture_state();
	}
	
	log_stack_pop();
}

void terminate_renderer(void) {
	log_stack_push("Renderer");
	destroy_vulkan_render_objects();
	destroy_vulkan_objects();
	free_premade_models();
	log_stack_pop();
}

void render_frame(float tick_delta_time) {
	
	const vector3F_t camera_position = area_render_state_camera_position();
	const projection_bounds_t projection_bounds = area_render_state_projection_bounds();
	
	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		if (texture_state_animate(&render_object_texture_states[i]) == 2) {
			upload_draw_data(get_global_area_render_state());
		}
	}

	glfwPollEvents();
	draw_frame(tick_delta_time, camera_position, projection_bounds);
}

void settle_render_positions(void) {
	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		settle_render_position(render_object_positions + i);
	}
}

void upload_model(uint32_t slot, model_t model, const char *const texture_name) {

	log_stack_push("Renderer");
	if (slot >= num_render_object_slots) {
		logf_message(ERROR, "Error uploading model: render object slot (%u) exceeds total number of render object slots (%u).", slot, num_render_object_slots);
		return;
	}

	stage_model_data(slot, model);
	string_t texture_id = new_string(256, texture_name);
	const texture_state_t texture_state = make_new_texture_state(find_loaded_texture_handle(texture_id));
	destroy_string(&texture_id);
	swap_render_object_texture_state(slot, texture_state);
	upload_draw_data(get_global_area_render_state());
	
	log_stack_pop();
}