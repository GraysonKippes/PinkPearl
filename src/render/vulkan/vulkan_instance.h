#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

// Contains functionality for the vulkan instance and the debug callback

#include <vulkan/vulkan.h>

#include "util/string_array.h"

/* -- VULKAN INSTANCE -- */

typedef struct VulkanInstance {

	VkInstance handle;

	StringArray layer_names;

} VulkanInstance;

VulkanInstance create_vulkan_instance(void);

void destroy_vulkan_instance(VulkanInstance vulkan_instance);

void setup_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT *messenger_ptr);

void destroy_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT messenger);

/* -- WINDOW SURFACE -- */

typedef struct WindowSurface {
	
	VkSurfaceKHR vkSurface;
	
	VkInstance vkInstance;
	
} WindowSurface;

WindowSurface createWindowSurface(const VulkanInstance vulkanInstance);

void deleteWindowSurface(WindowSurface *const pWindowSurface);

#endif	// VULKAN_INSTANCE_H
