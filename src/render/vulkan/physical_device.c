#include "physical_device.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t num_device_extensions = 1;

static char *device_extensions[1] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};



bool check_physical_device_extension_support(physical_device_t physical_device) {

	uint32_t num_available_extensions = 0;
	vkEnumerateDeviceExtensionProperties(physical_device.m_handle, NULL, &num_available_extensions, NULL);
	VkExtensionProperties *available_extensions = malloc(num_available_extensions * sizeof(VkExtensionProperties));
	vkEnumerateDeviceExtensionProperties(physical_device.m_handle, NULL, &num_available_extensions, available_extensions);

	for (size_t i = 0; i < physical_device.m_num_extensions; ++i) {
		
		bool extension_found = false;
		for (size_t j = 0; j < num_available_extensions; ++j) {
			if (strcmp(physical_device.m_extensions[i], available_extensions[j].extensionName) == 0) {
				extension_found = true;
				break;
			}
		}

		if (!extension_found)
			return false;
	}

	free(available_extensions);
	return true;
}

queue_family_indices_t query_queue_family_indices(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {

	uint32_t num_queue_families = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, NULL);
	VkQueueFamilyProperties *queue_families = malloc(num_queue_families * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families);

	queue_family_indices_t queue_family_indices;

	for (uint32_t i = 0; i < num_queue_families; ++i) {

		VkQueueFamilyProperties queue_family = queue_families[i];

		if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && queue_family_indices.m_graphics_family_ptr == NULL) {
			queue_family_indices.m_graphics_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.m_graphics_family_ptr = i;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
		if (present_support && queue_family_indices.m_present_family_ptr == NULL) {
			queue_family_indices.m_present_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.m_present_family_ptr = i;
		}

		if ((queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && queue_family_indices.m_transfer_family_ptr == NULL) {
			queue_family_indices.m_transfer_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.m_transfer_family_ptr = i;
		}
	}

	return queue_family_indices;
}

bool is_queue_family_indices_complete(queue_family_indices_t queue_family_indices) {
	return queue_family_indices.m_graphics_family_ptr != NULL && queue_family_indices.m_present_family_ptr != NULL && queue_family_indices.m_transfer_family_ptr != NULL;
}

// Allocates on the heap, make sure to free the surface format and present mode arrays eventually.
swapchain_support_details_t query_swapchain_support_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
	
	swapchain_support_details_t details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.m_capabilities);

	uint32_t num_formats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, NULL);
	if (num_formats != 0) {
		details.m_formats = malloc(num_formats * sizeof(VkSurfaceFormatKHR));
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, details.m_formats);
	}

	uint32_t num_present_modes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, NULL);
	if (num_present_modes != 0) {
		details.m_present_modes = malloc(num_present_modes * sizeof(VkPresentModeKHR));
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, details.m_present_modes);
	}

	return details;
	

}

// Rates a physical device based on certain criteria. Returns the score.
// A negative score means that the device is unsuitable for use.
int rate_physical_device(physical_device_t physical_device) {
	
	int score = 0;

	if (!check_physical_device_extension_support(physical_device))
		return -1;

	if (!physical_device.m_features.geometryShader)
		return -2;

	switch(physical_device.m_properties.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:	// Dedicated graphics card 
			score += 2000; 
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: 
			score += 1000; 
			break;
	}

	return score;
}

physical_device_t select_physical_device(VkInstance vk_instance, VkSurfaceKHR surface) {

	uint32_t num_physical_devices = 0;
	vkEnumeratePhysicalDevices(vk_instance, &num_physical_devices, NULL);
	if (num_physical_devices == 0) {
		physical_device_t physical_device;
		physical_device.m_handle = VK_NULL_HANDLE;
		return physical_device;
	}

	VkPhysicalDevice *physical_devices = malloc(num_physical_devices * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(vk_instance, &num_physical_devices, physical_devices);

	physical_device_t selected_physical_device;
	int selected_score = 0;

	for (uint32_t i = 0; i < num_physical_devices; ++i) {
		
		physical_device_t physical_device;
		physical_device.m_handle = physical_devices[i];
		vkGetPhysicalDeviceProperties(physical_device.m_handle, &physical_device.m_properties);
		vkGetPhysicalDeviceFeatures(physical_device.m_handle, &physical_device.m_features);
		physical_device.m_queue_family_indices = query_queue_family_indices(physical_device.m_handle, surface);
		physical_device.m_swapchain_support_details = query_swapchain_support_details(physical_device.m_handle, surface);

		int device_score = rate_physical_device(physical_device);
		if (device_score > selected_score) {
			selected_physical_device = physical_device;
			selected_score = device_score;
		}

	}

	selected_physical_device.m_extensions = device_extensions;
	selected_physical_device.m_num_extensions = num_device_extensions;

	free(physical_devices);
	return selected_physical_device;
}

void free_physical_device(physical_device_t physical_device) {
	free(physical_device.m_queue_family_indices.m_graphics_family_ptr);
	free(physical_device.m_queue_family_indices.m_present_family_ptr);
	free(physical_device.m_queue_family_indices.m_transfer_family_ptr);
	free(physical_device.m_swapchain_support_details.m_formats);
	free(physical_device.m_swapchain_support_details.m_present_modes);
}

bool find_physical_device_memory_type(VkPhysicalDevice physical_device, uint32_t filter, VkMemoryPropertyFlags properties, uint32_t *type_ptr) {

	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	if (type_ptr == NULL)
		return false;

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
		if (filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			*type_ptr = i;
			return true;
		}
	}

	return false;
}
