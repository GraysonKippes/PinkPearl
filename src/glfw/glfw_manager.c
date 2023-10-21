#include "glfw_manager.h"

#include "debug.h"
#include "log/logging.h"

#define WINDOW_WIDTH	960
#define WINDOW_HEIGHT	540

static GLFWwindow *window = NULL;

void init_GLFW(void) {

	log_message(INFO, "Initializing GLFW...");

	if (glfwInit() != GLFW_TRUE) {
		log_message(FATAL, "GLFW initialization failed.");
	}

	log_message(INFO, "Creating GLFW window...");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	const char *application_name = "Pink Pearl";

	if (debug_enabled) {
		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, application_name, NULL, NULL);
	}
	else {
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();

		const GLFWvidmode *mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		window = glfwCreateWindow(mode->width, mode->height, application_name, monitor, NULL);
	}

	glfwMakeContextCurrent(window);

	if (window == NULL) {
		log_message(FATAL, "GLFW window creation failed.");
	}
}

void terminate_GLFW(void) {
	glfwDestroyWindow(window);
	window = NULL;
	glfwTerminate();
}

GLFWwindow *get_application_window(void) {
	return window;
}
