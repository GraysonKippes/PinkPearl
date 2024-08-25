#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "util/string_array.h"

#define NUM_QUEUES 4



typedef struct QueueFamilyIndices {
	
	uint32_t *graphics_family_ptr;

	uint32_t *present_family_ptr;

	uint32_t *transfer_family_ptr;

	uint32_t *compute_family_ptr;

} QueueFamilyIndices;



typedef struct SwapchainSupportDetails {
	
	VkSurfaceCapabilitiesKHR capabilities;

	size_t num_formats;
	VkSurfaceFormatKHR *formats;

	size_t num_present_modes;
	VkPresentModeKHR *present_modes;

} SwapchainSupportDetails;



typedef struct PhysicalDevice {
	
	VkPhysicalDevice handle;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;

	QueueFamilyIndices queue_family_indices;
	SwapchainSupportDetails swapchain_support_details;
	string_array_t extension_names;

} PhysicalDevice;



// Selects a physical device from all the physical devices available on the user's machine.
PhysicalDevice select_physical_device(VkInstance vk_instance, VkSurfaceKHR surface);

// Frees all the arrays inside the physical device struct.
void destroy_physical_device(PhysicalDevice physical_device);

bool check_device_validation_layer_support(VkPhysicalDevice physical_device, string_array_t required_layer_names);

// Attempts to find the index to the memory type in the physical device. Returns true if the memory type was found and stored in type_ptr, false otherwise.
bool find_physical_device_memory_type(VkPhysicalDevice physical_device, uint32_t filter, VkMemoryPropertyFlags properties, uint32_t *type_ptr);

#endif	// PHYSICAL_DEVICE_H
