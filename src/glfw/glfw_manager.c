#include "glfw_manager.h"

#define WINDOW_WIDTH	960
#define WINDOW_HEIGHT	540

static GLFWwindow *window = NULL;

void init_GLFW(void) {
	
	if (glfwInit() != GLFW_TRUE) {
		// TODO - error handling
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	const char *application_name = "Pink Pearl";

	if (0) {
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

	if (window == NULL) {
		// TODO - error handling
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
