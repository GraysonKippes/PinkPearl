#include "physical_device.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "log/Logger.h"
#include "util/bit.h"



static const uint32_t num_device_extensions = 1;

static const char *device_extensions[1] = {
	
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};



void list_physical_device_memories(VkPhysicalDevice physical_device);

bool check_physical_device_extension_support(physical_device_t physical_device) {

	uint32_t num_available_extensions = 0;
	vkEnumerateDeviceExtensionProperties(physical_device.handle, nullptr, &num_available_extensions, nullptr);
	VkExtensionProperties *available_extensions = malloc(num_available_extensions * sizeof(VkExtensionProperties));
	vkEnumerateDeviceExtensionProperties(physical_device.handle, nullptr, &num_available_extensions, available_extensions);

	for (size_t i = 0; i < physical_device.extension_names.num_strings; ++i) {
		
		bool extension_found = false;

		for (size_t j = 0; j < num_available_extensions; ++j) {
			if (strcmp(physical_device.extension_names.strings[i], available_extensions[j].extensionName) == 0) {
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

bool check_device_validation_layer_support(VkPhysicalDevice physical_device, string_array_t required_layer_names) {

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Checking device validation layer support...");

	// If no layers are required, then no support is needed; therefore, return true.
	if (is_string_array_empty(required_layer_names)) {
		return true;
	}

	uint32_t num_available_layers = 0;
	vkEnumerateDeviceLayerProperties(physical_device, &num_available_layers, nullptr);

	// If the number of available layers is less than the number of required layers, then logically not all layers can be supported;
	// 	therefore, return false.
	if (num_available_layers < required_layer_names.num_strings) {
		return false;
	}

	VkLayerProperties *available_layers = calloc(num_available_layers, sizeof(VkLayerProperties));
	vkEnumerateDeviceLayerProperties(physical_device, &num_available_layers, available_layers);

	for (size_t i = 0; i < required_layer_names.num_strings; ++i) {

		const char *layer_name = required_layer_names.strings[i];
		bool layer_found = false;

		for (size_t j = 0; j < num_available_layers; ++j) {
			VkLayerProperties available_layer = available_layers[j];
			if (strcmp(layer_name, available_layer.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
			return false;
	}

	free(available_layers);
	available_layers = nullptr;

	return true;
}

queue_family_indices_t query_queue_family_indices(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {

	uint32_t num_queue_families = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, nullptr);
	VkQueueFamilyProperties *queue_families = malloc(num_queue_families * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families);

	queue_family_indices_t queue_family_indices;
	queue_family_indices.graphics_family_ptr = nullptr;
	queue_family_indices.present_family_ptr = nullptr;
	queue_family_indices.transfer_family_ptr = nullptr;
	queue_family_indices.compute_family_ptr = nullptr;

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Querying queue family indices...");

	for (uint32_t i = 0; i < num_queue_families; ++i) {

		VkQueueFamilyProperties queue_family = queue_families[i];

		VkBool32 graphics_support = TEST_MASK(queue_family.queueFlags, VK_QUEUE_GRAPHICS_BIT);
		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
		VkBool32 transfer_support = TEST_MASK(queue_family.queueFlags, VK_QUEUE_TRANSFER_BIT);
		VkBool32 compute_support = TEST_MASK(queue_family.queueFlags, VK_QUEUE_COMPUTE_BIT);

		if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && queue_family_indices.graphics_family_ptr == nullptr) {
			queue_family_indices.graphics_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.graphics_family_ptr = i;
		}

		if (present_support && queue_family_indices.present_family_ptr == nullptr) {
			queue_family_indices.present_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.present_family_ptr = i;
		}

		if ((queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && queue_family_indices.transfer_family_ptr == nullptr) {
			queue_family_indices.transfer_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.transfer_family_ptr = i;
		}

		if ((queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) && queue_family_indices.compute_family_ptr == nullptr) {
			queue_family_indices.compute_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.compute_family_ptr = i;
		}
	}

	return queue_family_indices;
}

bool is_queue_family_indices_complete(queue_family_indices_t queue_family_indices) {
	return queue_family_indices.graphics_family_ptr != nullptr 
		&& queue_family_indices.present_family_ptr != nullptr 
		&& queue_family_indices.transfer_family_ptr != nullptr
		&& queue_family_indices.compute_family_ptr != nullptr;
}

// Allocates on the heap, make sure to free the surface format and present mode arrays eventually.
swapchain_support_details_t query_swapchain_support_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
	
	swapchain_support_details_t details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);

	uint32_t num_formats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, nullptr);
	if (num_formats != 0) {
		details.num_formats = num_formats;
		details.formats = malloc(num_formats * sizeof(VkSurfaceFormatKHR));
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, details.formats);
	}

	uint32_t num_present_modes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, nullptr);
	if (num_present_modes != 0) {
		details.num_present_modes = num_present_modes;
		details.present_modes = malloc(num_present_modes * sizeof(VkPresentModeKHR));
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, details.present_modes);
	}

	return details;
	

}

// Rates a physical device based on certain criteria. Returns the score.
// A negative score means that the device is unsuitable for use.
int rate_physical_device(physical_device_t physical_device) {

	int score = 0;

	if (!check_physical_device_extension_support(physical_device))
		return -1;

	if (!physical_device.features.geometryShader)
		return -2;

	if (!is_queue_family_indices_complete(physical_device.queue_family_indices))
		return -3;

	switch(physical_device.properties.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:	// Dedicated graphics card 
			score += 2000; 
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: 
			score += 1000; 
			break;
	}

	return score;
}

physical_device_t make_new_physical_device(void) {
	return (physical_device_t){
		.handle = VK_NULL_HANDLE,
		.queue_family_indices.graphics_family_ptr = nullptr,
		.queue_family_indices.present_family_ptr = nullptr,
		.queue_family_indices.transfer_family_ptr = nullptr,
		.queue_family_indices.compute_family_ptr = nullptr,
		.swapchain_support_details.formats = nullptr,
		.swapchain_support_details.present_modes = nullptr,
		.extension_names.num_strings = 0,
		.extension_names.strings = nullptr
	};
}

physical_device_t select_physical_device(VkInstance vulkan_instance, VkSurfaceKHR surface) {

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Selecting physical device...");

	uint32_t num_physical_devices = 0;
	vkEnumeratePhysicalDevices(vulkan_instance, &num_physical_devices, nullptr);
	if (num_physical_devices == 0) {
		physical_device_t physical_device;
		physical_device.handle = VK_NULL_HANDLE;
		return physical_device;
	}

	VkPhysicalDevice *physical_devices = malloc(num_physical_devices * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(vulkan_instance, &num_physical_devices, physical_devices);

	physical_device_t selected_physical_device = make_new_physical_device();
	int selected_score = 0;

	for (uint32_t i = 0; i < num_physical_devices; ++i) {
	
		physical_device_t physical_device = make_new_physical_device();
		physical_device.handle = physical_devices[i];
		vkGetPhysicalDeviceProperties(physical_device.handle, &physical_device.properties);

		logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Rating physical device: %s", physical_device.properties.deviceName);

		list_physical_device_memories(physical_device.handle);

		vkGetPhysicalDeviceFeatures(physical_device.handle, &physical_device.features);
		physical_device.queue_family_indices = query_queue_family_indices(physical_device.handle, surface);
		physical_device.swapchain_support_details = query_swapchain_support_details(physical_device.handle, surface);
		
		physical_device.extension_names.num_strings = num_device_extensions;
		physical_device.extension_names.strings = device_extensions;

		int device_score = rate_physical_device(physical_device);

		// TODO - proper cleanup of rejected physical devices
		if (device_score > selected_score) {
			selected_physical_device = physical_device;
			selected_score = device_score;
		}
		else {
			//destroy_physical_device(physical_device);
		}
	}

	VkPhysicalDeviceMaintenance4Properties physical_device_maintenance = { 0 };
	physical_device_maintenance.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES;
	physical_device_maintenance.pNext = nullptr;
	physical_device_maintenance.maxBufferSize = 0;

	VkPhysicalDeviceProperties2 physical_device_properties = { 0 };
	physical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	physical_device_properties.pNext = &physical_device_maintenance;
	physical_device_properties.properties = (VkPhysicalDeviceProperties){ 0 };

	vkGetPhysicalDeviceProperties2(selected_physical_device.handle, &physical_device_properties);

	free(physical_devices);
	return selected_physical_device;
}

void destroy_physical_device(physical_device_t physical_device) {
	free(physical_device.queue_family_indices.graphics_family_ptr);
	free(physical_device.queue_family_indices.present_family_ptr);
	free(physical_device.queue_family_indices.transfer_family_ptr);
	free(physical_device.queue_family_indices.compute_family_ptr);
	free(physical_device.swapchain_support_details.formats);
	free(physical_device.swapchain_support_details.present_modes);
}

bool find_physical_device_memory_type(VkPhysicalDevice physical_device, uint32_t filter, VkMemoryPropertyFlags properties, uint32_t *type_ptr) {

	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	if (type_ptr == nullptr)
		return false;

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
		if (filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			*type_ptr = i;
			return true;
		}
	}

	return false;
}

// Diagnostic
void list_physical_device_memories(VkPhysicalDevice physical_device) {

	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {

		VkMemoryType memory_type = memory_properties.memoryTypes[i];

		uint32_t heap_index = memory_type.heapIndex;
		VkDeviceSize heap_size = memory_properties.memoryHeaps[heap_index].size;
		bool heap_device_local = (memory_properties.memoryHeaps[heap_index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) >= 1;

		bool device_local = (memory_type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) >= 1;
		bool host_visible = (memory_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) >= 1;
		bool host_coherent = (memory_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) >= 1;
		bool host_cached = (memory_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) >= 1;
	}
}
