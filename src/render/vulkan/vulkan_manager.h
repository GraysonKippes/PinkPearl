#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

// This file contains all the vulkan objects and functions to create them and destroy them.

// Creates all Vulkan objects needed for the rendering system.
void create_vulkan_objects(void);

// Destroys the Vulkan objects created for the rendering system.
void destroy_vulkan_objects(void);

void render_frame(double delta_time);

#endif	// VULKAN_MANAGER_H
