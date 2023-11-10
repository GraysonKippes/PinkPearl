#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

// Contains functionality for the vulkan instance and the debug callback

#include <vulkan/vulkan.h>

#include "util/string_array.h"

typedef struct vulkan_instance_t {

	VkInstance m_handle;

	string_array_t m_layer_names;

} vulkan_instance_t;

void create_vulkan_instance(VkInstance *vulkan_instance_ptr);

void setup_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT *messenger_ptr);

void destroy_vulkan_instance(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT messenger);

#endif	// VULKAN_INSTANCE_H
