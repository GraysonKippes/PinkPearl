#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include "config.h"

#include "debug.h"
#include "game/game.h"
#include "glfw/glfw_manager.h"
#include "log/logging.h"
#include "render/model.h"
#include "render/renderer.h"
#include "render/vulkan/vulkan_manager.h"

int main(void) {

	logf_message(INFO, "Running Pink Pearl version %u.%u", PinkPearl_VERSION_MAJOR, PinkPearl_VERSION_MINOR);
	logf_message(INFO, "C standard: %lu", __STDC_VERSION__);
	if (debug_enabled) {
		log_message(WARNING, "Debug mode enabled.");
	}

	logf_message(VERBOSE, "Resource path: \"%s\"", RESOURCE_PATH);

	init_GLFW();
	create_vulkan_objects();
	make_premade_models();
	start_game();

	log_message(INFO, "Ready to play Pink Pearl!");

	while(!glfwWindowShouldClose(get_application_window())) {
		glfwPollEvents();
		render_frame(0.0);
	}

	free_premade_models();

	destroy_vulkan_objects();

	terminate_GLFW();

	log_message(INFO, "Stopping Pink Pearl. Goodbye!");
	
	return 0;
}
