#include "GLFWManager.h"

#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "log/Logger.h"
#include "render/stb/image_data.h"
#include "InputManager.h"

static const int windowWidthDefault = 960;
static const int windowHeightDefault = 600;

static GLFWwindow *window = nullptr;

static void glfwErrorCallback(int code, const char *description);

void initGLFW(void) {
	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Initializing GLFW...");

	if (glfwInit() != GLFW_TRUE) {
		logMsg(loggerSystem, LOG_LEVEL_FATAL, "Initializing GLFW: GLFW initialization failed.");
	}
	glfwSetErrorCallback(glfwErrorCallback);

	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Creating GLFW window...");
	
	char appName[64];
	snprintf(appName, 64, "%s %i.%i", APP_NAME, PinkPearl_VERSION_MAJOR, PinkPearl_VERSION_MINOR);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	if (debug_enabled) {
		window = glfwCreateWindow(windowWidthDefault, windowHeightDefault, appName, nullptr, nullptr);
	} else {
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		window = glfwCreateWindow(mode->width, mode->height, appName, monitor, nullptr);
	}

	if (!window) {
		logMsg(loggerSystem, LOG_LEVEL_FATAL, "Initializing GLFW: window creation failed.");
	}
	
	initInputManager(window);

	GLFWimage icon = load_glfw_image(RESOURCE_PATH "assets/textures/icon.png");
	glfwSetWindowIcon(window, 1, &icon);
	
	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Initialized GLFW.");
}

void terminateGLFW(void) {
	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Terminating GLFW...");
	glfwDestroyWindow(window);
	window = nullptr;
	glfwTerminate();
	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Terminated GLFW.");
}

GLFWwindow *getAppWindow(void) {
	return window;
}

bool shouldAppWindowClose(void) {
	return glfwWindowShouldClose(window);
}

static void glfwErrorCallback(int code, const char *description) {
	logMsg(loggerSystem, LOG_LEVEL_ERROR, "GLFW error (%i): %s", code, description);
}
