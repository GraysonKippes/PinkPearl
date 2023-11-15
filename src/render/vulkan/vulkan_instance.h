#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

// Contains functionality for the vulkan instance and the debug callback

#include <vulkan/vulkan.h>

#include "util/string_array.h"

typedef struct vulkan_instance_t {

	VkInstance handle;

	string_array_t layer_names;

} vulkan_instance_t;

vulkan_instance_t create_vulkan_instance(void);

void destroy_vulkan_instance(vulkan_instance_t vulkan_instance);

void setup_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT *messenger_ptr);

void destroy_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT messenger);

#endif	// VULKAN_INSTANCE_H
