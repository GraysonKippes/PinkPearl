#include "physical_device.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "log/Logger.h"

static const uint32_t num_device_extensions = 1;

static const char *device_extensions[1] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool check_physical_device_extension_support(PhysicalDevice physical_device) {

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

static QueueFamilyIndices getQueueFamilyIndices(VkPhysicalDevice physical_device, WindowSurface windowSurface) {

	uint32_t num_queue_families = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, nullptr);
	VkQueueFamilyProperties *queue_families = malloc(num_queue_families * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families);

	QueueFamilyIndices queue_family_indices;
	queue_family_indices.graphics_family_ptr = nullptr;
	queue_family_indices.present_family_ptr = nullptr;
	queue_family_indices.transfer_family_ptr = nullptr;
	queue_family_indices.compute_family_ptr = nullptr;

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Querying queue family indices...");

	for (uint32_t i = 0; i < num_queue_families; ++i) {

		VkQueueFamilyProperties queue_family = queue_families[i];

		VkBool32 graphicsSupport = queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT;
		VkBool32 transferSupport = queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT;
		VkBool32 computeSupport = queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT;
		
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, windowSurface.vkSurface, &presentSupport);

		if (graphicsSupport && queue_family_indices.graphics_family_ptr == nullptr) {
			queue_family_indices.graphics_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.graphics_family_ptr = i;
		}

		if (presentSupport && queue_family_indices.present_family_ptr == nullptr) {
			queue_family_indices.present_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.present_family_ptr = i;
		}

		if (transferSupport && !(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && queue_family_indices.transfer_family_ptr == nullptr) {
			queue_family_indices.transfer_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.transfer_family_ptr = i;
		}

		if (computeSupport && queue_family_indices.compute_family_ptr == nullptr) {
			queue_family_indices.compute_family_ptr = malloc(sizeof(uint32_t));
			*queue_family_indices.compute_family_ptr = i;
		}
	}

	return queue_family_indices;
}

bool is_queue_family_indices_complete(QueueFamilyIndices queue_family_indices) {
	return queue_family_indices.graphics_family_ptr != nullptr 
		&& queue_family_indices.present_family_ptr != nullptr 
		&& queue_family_indices.transfer_family_ptr != nullptr
		&& queue_family_indices.compute_family_ptr != nullptr;
}

// Allocates on the heap, make sure to free the surface format and present mode arrays eventually.
static SwapchainSupportDetails getSwapchainSupportDetails(VkPhysicalDevice physical_device, WindowSurface windowSurface) {
	
	SwapchainSupportDetails details = { };

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, windowSurface.vkSurface, &details.capabilities);

	uint32_t num_formats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, windowSurface.vkSurface, &num_formats, nullptr);
	if (num_formats != 0) {
		details.num_formats = num_formats;
		details.formats = malloc(num_formats * sizeof(VkSurfaceFormatKHR));
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, windowSurface.vkSurface, &num_formats, details.formats);
	}

	uint32_t num_present_modes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, windowSurface.vkSurface, &num_present_modes, nullptr);
	if (num_present_modes != 0) {
		details.num_present_modes = num_present_modes;
		details.present_modes = malloc(num_present_modes * sizeof(VkPresentModeKHR));
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, windowSurface.vkSurface, &num_present_modes, details.present_modes);
	}

	return details;
}

// Rates a physical device based on certain criteria. Returns the score.
// A negative score means that the device is unsuitable for use.
int rate_physical_device(PhysicalDevice physical_device) {

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

PhysicalDevice make_new_physical_device(void) {
	return (PhysicalDevice){
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

PhysicalDevice select_physical_device(const VulkanInstance vulkanInstance, const WindowSurface windowSurface) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Selecting physical device...");

	uint32_t num_physical_devices = 0;
	vkEnumeratePhysicalDevices(vulkanInstance.handle, &num_physical_devices, nullptr);
	if (num_physical_devices == 0) {
		PhysicalDevice physical_device;
		physical_device.handle = VK_NULL_HANDLE;
		return physical_device;
	}

	VkPhysicalDevice *physical_devices = malloc(num_physical_devices * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(vulkanInstance.handle, &num_physical_devices, physical_devices);

	PhysicalDevice selected_physical_device = make_new_physical_device();
	int selected_score = 0;

	for (uint32_t i = 0; i < num_physical_devices; ++i) {
	
		PhysicalDevice physical_device = make_new_physical_device();
		physical_device.handle = physical_devices[i];
		vkGetPhysicalDeviceProperties(physical_device.handle, &physical_device.properties);

		//logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Rating physical device: %s", physical_device.properties.deviceName);

		vkGetPhysicalDeviceFeatures(physical_device.handle, &physical_device.features);
		physical_device.queue_family_indices = getQueueFamilyIndices(physical_device.handle, windowSurface);
		physical_device.swapchain_support_details = getSwapchainSupportDetails(physical_device.handle, windowSurface);
		
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

void destroy_physical_device(PhysicalDevice physical_device) {
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
