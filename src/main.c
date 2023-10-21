#include <stdio.h>

#include "debug.h"
#include "glfw/glfw_manager.h"
#include "log/logging.h"
#include "render/vulkan/vulkan_manager.h"

int main(void) {

	log_message(INFO, "Running Pink Pearl...");

	if (debug_enabled)
		log_message(INFO, "Debug mode enabled.");

	init_GLFW();

	create_vulkan_objects();

	while(!glfwWindowShouldClose(get_application_window())) {
		glfwPollEvents();
		render_frame(0.0);
	}

	destroy_vulkan_objects();

	terminate_GLFW();

	log_message(INFO, "Stopping Pink Pearl. Goodbye!");
	
	return 0;
}
