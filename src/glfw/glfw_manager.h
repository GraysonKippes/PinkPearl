#ifndef GLFW_MANAGER_H
#define GLFW_MANAGER_H

#include <stdbool.h>

/* define GLFW_INCLUDE_VULKAN or
 * include a Vulkan header
 * before including this header to use GLFW-Vulkan functions.
*/

#include <GLFW/glfw3.h>

// Initializes GLFW and creates the application window.
void init_GLFW(void);

// Destroys the application window and terminations GLFW.
void terminate_GLFW(void);

// Returns the handle to the application window.
GLFWwindow *get_application_window(void);

bool should_application_window_close(void);

#endif	// GLFW_MANAGER_H
