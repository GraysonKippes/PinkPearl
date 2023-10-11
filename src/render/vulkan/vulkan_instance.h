#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

// Contains functionality for the vulkan instance and the debug callback

#include <vulkan/vulkan.h>

void create_vulkan_instance(VkInstance *instance_ptr);

void setup_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT *messenger_ptr);

void destroy_vulkan_instance(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT messenger);

#endif	// VULKAN_INSTANCE_H
