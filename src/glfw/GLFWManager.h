#ifndef GLFW_MANAGER_H
#define GLFW_MANAGER_H

/* define GLFW_INCLUDE_VULKAN or
 * include a Vulkan header
 * before including this header to use GLFW-Vulkan functions.
*/

#include <stdbool.h>
#include <GLFW/glfw3.h>

// Initializes GLFW and creates the application window.
void initGLFW(void);

// Destroys the application window and terminations GLFW.
void terminateGLFW(void);

// Returns the handle to the application window.
GLFWwindow *getAppWindow(void);

bool shouldAppWindowClose(void);

#endif	// GLFW_MANAGER_H
