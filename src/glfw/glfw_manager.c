#include "glfw_manager.h"

#include "config.h"
#include "debug.h"
#include "log/logging.h"
#include "render/stb/image_data.h"

static const int window_width_default = 960;
static const int window_height_default = 600;

static GLFWwindow *window = NULL;

void glfw_error_callback(int code, const char *description);

void init_GLFW(void) {

	log_message(INFO, "Initializing GLFW...");

	if (glfwInit() != GLFW_TRUE) {
		log_message(FATAL, "GLFW initialization failed.");
	}

	glfwSetErrorCallback(glfw_error_callback);

	log_message(VERBOSE, "Creating GLFW window...");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	const char *application_name = APP_NAME;

	if (debug_enabled) {
		window = glfwCreateWindow(window_width_default, window_height_default, APP_NAME, NULL, NULL);
	}
	else {
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();

		const GLFWvidmode *mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		window = glfwCreateWindow(mode->width, mode->height, APP_NAME, monitor, NULL);
	}

	if (window == NULL) {
		log_message(FATAL, "GLFW window creation failed.");
	}

	GLFWimage icons[1];
	icons[0] = load_glfw_image(RESOURCE_PATH "assets/textures/icon.png");

	// TODO - get window icon to work
	//glfwSetWindowIcon(window, 1, icons);
}

void terminate_GLFW(void) {
	glfwDestroyWindow(window);
	window = NULL;
	glfwTerminate();
}

GLFWwindow *get_application_window(void) {
	return window;
}

void glfw_error_callback(int code, const char *description) {
	logf_message(ERROR, "GLFW error (%i): %s", code, description);
}
