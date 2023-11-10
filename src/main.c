#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include "debug.h"
#include "glfw/glfw_manager.h"
#include "log/logging.h"
#include "render/model.h"
#include "render/renderer.h"
#include "render/vulkan/vulkan_manager.h"

int main(void) {

	log_message(INFO, "Running Pink Pearl...");

	logf_message(INFO, "C standard: %lu", __STDC_VERSION__);

	if (debug_enabled) {
		log_message(WARNING, "Debug mode enabled.");
	}

	init_GLFW();

	create_vulkan_objects();

	make_premade_models();

	// TEST
	
	time_t start_time = time(NULL);

	bool staged = false;

	model_t model0 = get_premade_model(0);

	stage_model_data(0, model0);

	projection_bounds_t projection_bounds = { 
		.m_left = -12.0F, .m_right = 12.0F, 
		.m_top = 7.5F, .m_bottom = -7.5F, 
		.m_near = 15.0F, .m_far = -15.0F 
	};

	projection_bounds_t projection_bounds2 = { 
		.m_left = -12.0F, .m_right = 12.0F, 
		.m_bottom = -7.5F, .m_top = 7.5F, 
		.m_near = 15.0F, .m_far = -15.0F 
	};

	update_projection_bounds(projection_bounds_8x5);
	// UNTEST

	log_message(INFO, "Ready to play Pink Pearl!");

	while(!glfwWindowShouldClose(get_application_window())) {

		if (time(NULL) - start_time > 5 && !staged) {
			staged = true;
		}

		glfwPollEvents();
		render_frame(0.0);
	}

	free_premade_models();

	destroy_vulkan_objects();

	terminate_GLFW();

	log_message(INFO, "Stopping Pink Pearl. Goodbye!");
	
	return 0;
}
