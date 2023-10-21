#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#define NUM_QUEUES 4

typedef struct queue_family_indices_t {
	
	uint32_t *m_graphics_family_ptr;

	uint32_t *m_present_family_ptr;

	uint32_t *m_transfer_family_ptr;

	uint32_t *m_compute_family_ptr;

} queue_family_indices_t;



typedef struct swapchain_support_details_t {
	
	VkSurfaceCapabilitiesKHR m_capabilities;

	size_t m_num_formats;
	VkSurfaceFormatKHR *m_formats;

	size_t m_num_present_modes;
	VkPresentModeKHR *m_present_modes;

} swapchain_support_details_t;



typedef struct physical_device_t {
	
	VkPhysicalDevice m_handle;
	VkPhysicalDeviceProperties m_properties;
	VkPhysicalDeviceFeatures m_features;

	queue_family_indices_t m_queue_family_indices;

	swapchain_support_details_t m_swapchain_support_details;

	const char **m_extensions;
	uint32_t m_num_extensions;

} physical_device_t;



// Selects a physical device from all the physical devices available on the user's machine.
physical_device_t select_physical_device(VkInstance vk_instance, VkSurfaceKHR surface);

// Frees all the arrays inside the physical device struct.
void destroy_physical_device(physical_device_t physical_device);

// Attempts to find the index to the memory type in the physical device. Returns true if the memory type was found and stored in type_ptr, false otherwise.
bool find_physical_device_memory_type(VkPhysicalDevice physical_device, uint32_t filter, VkMemoryPropertyFlags properties, uint32_t *type_ptr);

#endif	// PHYSICAL_DEVICE_H
