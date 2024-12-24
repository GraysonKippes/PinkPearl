#include "GLFWManager.h"

#include <assert.h>
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "log/Logger.h"
#include "render/stb/ImageData.h"
#include "InputManager.h"

#define DEFAULT_WINDOW_WIDTH 	960
#define DEFAULT_WINDOW_HEIGHT	600

typedef struct AppWindow {
	GLFWwindow *pHandle;
	int32_t width;
	int32_t height;
} AppWindow;

static AppWindow appWindow;

static void glfwErrorCallback(int code, const char *description);

AppWindow createWindow(const int32_t width, const int32_t height, const char title[const], const ImageData icon, const bool fullscreen);

void deleteWindow(AppWindow *const pWindow);

void initGLFW(void) {
	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Initializing GLFW...");

	if (glfwInit() != GLFW_TRUE) {
		logMsg(loggerSystem, LOG_LEVEL_FATAL, "Initializing GLFW: GLFW initialization failed.");
	}
	glfwSetErrorCallback(glfwErrorCallback);

	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Creating GLFW window...");
	
	char appName[64];
	snprintf(appName, 64, "%s %i.%i", APP_NAME, PinkPearl_VERSION_MAJOR, PinkPearl_VERSION_MINOR);

	ImageData icon = loadImageData(RESOURCE_PATH "assets/textures/icon.png", COLOR_TRANSPARENT);
	
	appWindow = createWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, appName, icon, !debug_enabled);
	if (!appWindow.pHandle) {
		logMsg(loggerSystem, LOG_LEVEL_FATAL, "Initializing GLFW: window creation failed.");
	}
	
	deleteImageData(&icon);
	
	initInputManager(appWindow.pHandle);
	
	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Initialized GLFW.");
}

void terminateGLFW(void) {
	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Terminating GLFW...");
	deleteWindow(&appWindow);
	glfwTerminate();
	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Terminated GLFW.");
}

AppWindow createWindow(const int32_t width, const int32_t height, const char title[const], const ImageData icon, const bool fullscreen) {
	
	AppWindow window = { };

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	
	if (fullscreen) {
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		window.pHandle = glfwCreateWindow(mode->width, mode->height, title, monitor, nullptr);
		window.width = mode->width;
		window.height = mode->height;
	} else {
		window.pHandle = glfwCreateWindow(width, height, title, nullptr, nullptr);
		window.width = width;
		window.height = height;
	}
	
	if (!window.pHandle) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Creating window: failed to create window.");
		return (AppWindow){ };
	}
	
	const GLFWimage img = {
		.width = icon.width,
		.height = icon.height,
		.pixels = icon.pPixels
	};
	glfwSetWindowIcon(window.pHandle, 1, &img);
	
	return window;
}

void deleteWindow(AppWindow *const pWindow) {
	assert(pWindow);
	glfwDestroyWindow(pWindow->pHandle);
	pWindow->pHandle = nullptr;
	pWindow->width = 0;
	pWindow->height = 0;
}

GLFWwindow *getAppWindow(void) {
	return appWindow.pHandle;
}

bool shouldAppWindowClose(void) {
	return glfwWindowShouldClose(appWindow.pHandle);
}

void getCursorPosition(double *const pPosX, double *const pPosY) {
	assert(pPosX && pPosY);
	double x = 0.0, y = 0.0;
	glfwGetCursorPos(appWindow.pHandle, &x, &y);
	x /= appWindow.width; // Normalize to [0, 1]
	y /= appWindow.height;
	x -= 0.5; // Shift to [-0.5, 0.5]
	y -= 0.5;
	x *= 2.0; // Scale to [-1, 1]
	y *= -2.0; // Flip Y position
	*pPosX = x;
	*pPosY = y;
}

static void glfwErrorCallback(int code, const char *description) {
	logMsg(loggerSystem, LOG_LEVEL_ERROR, "GLFW error (%i): %s", code, description);
}
