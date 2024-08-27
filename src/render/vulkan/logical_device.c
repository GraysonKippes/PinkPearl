#include "logical_device.h"

#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "log/Logger.h"
#include "util/string_array.h"

// Returns a pointer-array that must be freed by the caller.
VkDeviceQueueCreateInfo *make_queue_create_infos(QueueFamilyIndices queue_family_indices, uint32_t *num_queue_create_infos) {

	// Pack the queue family indices into an array for easy iteration.
	uint32_t indices_arr[NUM_QUEUES];
	indices_arr[0] = *queue_family_indices.graphics_family_ptr;
	indices_arr[1] = *queue_family_indices.present_family_ptr;
	indices_arr[2] = *queue_family_indices.transfer_family_ptr;
	indices_arr[3] = *queue_family_indices.compute_family_ptr;

	uint32_t num_unique_queue_families = 0;
	uint32_t unique_queue_families[NUM_QUEUES];

	for (uint32_t i = 0; i < NUM_QUEUES; ++i) {

		uint32_t index = indices_arr[i];
		bool index_already_present = false;

		for (uint32_t j = i + 1; j < NUM_QUEUES; ++j) {
			if (index == indices_arr[j])
				index_already_present = true;
		}

		if (!index_already_present) {
			unique_queue_families[num_unique_queue_families++] = index;
		}
	}

	VkDeviceQueueCreateInfo *queue_create_infos = calloc(num_unique_queue_families, sizeof(VkDeviceQueueCreateInfo));

	static const float queue_priority = 1.0F;

	for (uint32_t i = 0; i < num_unique_queue_families; ++i) {
		queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_infos[i].queueFamilyIndex = unique_queue_families[i];
		queue_create_infos[i].queueCount = 1;
		queue_create_infos[i].pQueuePriorities = &queue_priority;
	}

	if (num_queue_create_infos) {
		*num_queue_create_infos = num_unique_queue_families;
	}

	return queue_create_infos;
}

void create_device(VulkanInstance vulkan_instance, PhysicalDevice physical_device, VkDevice *device_ptr) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating logical device...");

	uint32_t num_queue_create_infos = 0;
	VkDeviceQueueCreateInfo *queue_create_infos = make_queue_create_infos(physical_device.queueFamilyIndices, &num_queue_create_infos);

	VkPhysicalDeviceVulkan13Features features13 = { VK_FALSE };
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.pNext = nullptr;
	features13.synchronization2 = VK_TRUE;
	features13.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceVulkan12Features features12 = { VK_FALSE };
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.pNext = &features13;
	features12.drawIndirectCount = VK_TRUE;
	features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	features12.descriptorIndexing = VK_TRUE;
	features12.descriptorBindingPartiallyBound = VK_TRUE;
	features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
	features12.runtimeDescriptorArray = VK_TRUE;
	features12.scalarBlockLayout = VK_TRUE;
	features12.timelineSemaphore = VK_TRUE;
	
	VkPhysicalDeviceVulkan11Features features11 = { VK_FALSE };
	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	features11.pNext = &features12;
	features11.storageBuffer16BitAccess = VK_TRUE;
	features11.uniformAndStorageBuffer16BitAccess = VK_TRUE;
	features11.shaderDrawParameters = VK_TRUE;

	VkPhysicalDeviceFeatures2 features = { 
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &features11,
		.features = (VkPhysicalDeviceFeatures){ }
	};
	features.features.samplerAnisotropy = VK_TRUE;
	
	VkDeviceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &features,
		.flags = 0,
		.queueCreateInfoCount = num_queue_create_infos,
		.pQueueCreateInfos = queue_create_infos,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = physical_device.extensionNames.num_strings,
		.ppEnabledExtensionNames = physical_device.extensionNames.strings,
		.pEnabledFeatures = nullptr
	};

	// Compatibility
	if (debug_enabled) {
		if (!check_device_validation_layer_support(physical_device.vkPhysicalDevice, vulkan_instance.layer_names)) {
			logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Required validation layers not supported by device.");
			createInfo.enabledLayerCount = 0;
			createInfo.ppEnabledLayerNames = nullptr;
		}
		else {
			createInfo.enabledLayerCount = vulkan_instance.layer_names.num_strings;
			createInfo.ppEnabledLayerNames = vulkan_instance.layer_names.strings;
		}
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	VkResult result = vkCreateDevice(physical_device.vkPhysicalDevice, &createInfo, nullptr, device_ptr);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating logical device: logical device creation failed (error code: %i).", result);
		exit(1); // TODO - better error handling
	}

	free(queue_create_infos);
}
