#include "physical_device.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "log/Logger.h"

#define DEVICE_EXTENSION_COUNT 1

static const uint32_t deviceExtensionCount = DEVICE_EXTENSION_COUNT;

static const char *pDeviceExtensionNames[DEVICE_EXTENSION_COUNT] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const PhysicalDevice nullPhysicalDevice = {
	.vkPhysicalDevice = VK_NULL_HANDLE,
	.queueFamilyIndices.graphics_family_ptr = nullptr,
	.queueFamilyIndices.present_family_ptr = nullptr,
	.queueFamilyIndices.transfer_family_ptr = nullptr,
	.queueFamilyIndices.compute_family_ptr = nullptr,
	.swapchainSupportDetails.formats = nullptr,
	.swapchainSupportDetails.present_modes = nullptr,
	.extensionNames.num_strings = 0,
	.extensionNames.strings = nullptr
};

bool check_physical_device_extension_support(PhysicalDevice physical_device) {

	uint32_t num_available_extensions = 0;
	vkEnumerateDeviceExtensionProperties(physical_device.vkPhysicalDevice, nullptr, &num_available_extensions, nullptr);
	VkExtensionProperties *available_extensions = malloc(num_available_extensions * sizeof(VkExtensionProperties));
	vkEnumerateDeviceExtensionProperties(physical_device.vkPhysicalDevice, nullptr, &num_available_extensions, available_extensions);

	for (size_t i = 0; i < physical_device.extensionNames.num_strings; ++i) {
		
		bool extension_found = false;

		for (size_t j = 0; j < num_available_extensions; ++j) {
			if (strcmp(physical_device.extensionNames.strings[i], available_extensions[j].extensionName) == 0) {
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

bool check_device_validation_layer_support(VkPhysicalDevice physical_device, StringArray required_layer_names) {

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

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, nullptr);
	VkQueueFamilyProperties queueFamilies[queueFamilyCount];
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, queueFamilies);

	QueueFamilyIndices queueFamilyIndices = { };

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Querying queue family indices...");

	for (uint32_t i = 0; i < queueFamilyCount; ++i) {

		const VkQueueFamilyProperties queueFamily = queueFamilies[i];

		const VkBool32 graphicsSupport = queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT;
		const VkBool32 transferSupport = queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT;
		const VkBool32 computeSupport = queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT;
		
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, windowSurface.vkSurface, &presentSupport);

		if (graphicsSupport && queueFamilyIndices.graphics_family_ptr == nullptr) {
			queueFamilyIndices.graphics_family_ptr = malloc(sizeof(uint32_t));
			*queueFamilyIndices.graphics_family_ptr = i;
		}

		if (presentSupport && queueFamilyIndices.present_family_ptr == nullptr) {
			queueFamilyIndices.present_family_ptr = malloc(sizeof(uint32_t));
			*queueFamilyIndices.present_family_ptr = i;
		}

		if (transferSupport && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && queueFamilyIndices.transfer_family_ptr == nullptr) {
			queueFamilyIndices.transfer_family_ptr = malloc(sizeof(uint32_t));
			*queueFamilyIndices.transfer_family_ptr = i;
		}

		if (computeSupport && queueFamilyIndices.compute_family_ptr == nullptr) {
			queueFamilyIndices.compute_family_ptr = malloc(sizeof(uint32_t));
			*queueFamilyIndices.compute_family_ptr = i;
		}
	}

	return queueFamilyIndices;
}

bool is_queue_family_indices_complete(QueueFamilyIndices queueFamilyIndices) {
	return queueFamilyIndices.graphics_family_ptr != nullptr 
		&& queueFamilyIndices.present_family_ptr != nullptr 
		&& queueFamilyIndices.transfer_family_ptr != nullptr
		&& queueFamilyIndices.compute_family_ptr != nullptr;
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

	if (!is_queue_family_indices_complete(physical_device.queueFamilyIndices))
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

PhysicalDevice select_physical_device(const VulkanInstance vulkanInstance, const WindowSurface windowSurface) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Selecting physical device...");

	

	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(vulkanInstance.handle, &physicalDeviceCount, nullptr);
	if (physicalDeviceCount == 0) {
		return nullPhysicalDevice;
	}

	VkPhysicalDevice vkPhysicalDevices[physicalDeviceCount];
	vkEnumeratePhysicalDevices(vulkanInstance.handle, &physicalDeviceCount, vkPhysicalDevices);

	PhysicalDevice selectedPhysicalDevice = nullPhysicalDevice;
	int selectedScore = 0;

	for (uint32_t i = 0; i < physicalDeviceCount; ++i) {
	
		PhysicalDevice physicalDevice = nullPhysicalDevice;
		physicalDevice.vkPhysicalDevice = vkPhysicalDevices[i];
		
		vkGetPhysicalDeviceProperties(physicalDevice.vkPhysicalDevice, &physicalDevice.properties);
		vkGetPhysicalDeviceFeatures(physicalDevice.vkPhysicalDevice, &physicalDevice.features);
		
		physicalDevice.queueFamilyIndices = getQueueFamilyIndices(physicalDevice.vkPhysicalDevice, windowSurface);
		physicalDevice.swapchainSupportDetails = getSwapchainSupportDetails(physicalDevice.vkPhysicalDevice, windowSurface);
		
		physicalDevice.extensionNames.num_strings = deviceExtensionCount;
		physicalDevice.extensionNames.strings = pDeviceExtensionNames;

		int device_score = rate_physical_device(physicalDevice);
		if (device_score > selectedScore) {
			selectedPhysicalDevice = physicalDevice;
			selectedScore = device_score;
		}
	}

	return selectedPhysicalDevice;
}

void deletePhysicalDevice(PhysicalDevice *const pPhysicalDevice) {
	if (!pPhysicalDevice) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting physical device: pointer to physical device object is null.");
	}
	
	free(pPhysicalDevice->queueFamilyIndices.graphics_family_ptr);
	free(pPhysicalDevice->queueFamilyIndices.present_family_ptr);
	free(pPhysicalDevice->queueFamilyIndices.transfer_family_ptr);
	free(pPhysicalDevice->queueFamilyIndices.compute_family_ptr);
	free(pPhysicalDevice->swapchainSupportDetails.formats);
	free(pPhysicalDevice->swapchainSupportDetails.present_modes);
	
	*pPhysicalDevice = nullPhysicalDevice;
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
