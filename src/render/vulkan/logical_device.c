#include "logical_device.h"

#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "log/logging.h"
#include "util/string_array.h"

// Returns a pointer-array that must be freed by the caller.
VkDeviceQueueCreateInfo *make_queue_create_infos(queue_family_indices_t queue_family_indices, uint32_t *num_queue_create_infos) {

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

	if (num_queue_create_infos)
		*num_queue_create_infos = num_unique_queue_families;

	return queue_create_infos;
}

void create_device(vulkan_instance_t vulkan_instance, physical_device_t physical_device, VkDevice *device_ptr) {

	log_message(VERBOSE, "Creating logical device...");

	uint32_t num_queue_create_infos = 0;
	VkDeviceQueueCreateInfo *queue_create_infos = make_queue_create_infos(physical_device.queue_family_indices, &num_queue_create_infos);

	VkPhysicalDeviceVulkan13Features device_vk13_features = { VK_FALSE };
	device_vk13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	device_vk13_features.pNext = NULL;
	device_vk13_features.synchronization2 = VK_TRUE;

	VkPhysicalDeviceVulkan12Features device_vk12_features = { VK_FALSE };
	device_vk12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	device_vk12_features.pNext = &device_vk13_features;
	device_vk12_features.timelineSemaphore = VK_TRUE;

	VkPhysicalDeviceFeatures2 device_features = { 
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &device_vk12_features,
		.features = (VkPhysicalDeviceFeatures){ 0 }
	};
	device_features.features.samplerAnisotropy = VK_TRUE;
	
	VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &device_features,
		.flags = 0,
		.queueCreateInfoCount = num_queue_create_infos,
		.pQueueCreateInfos = queue_create_infos,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = physical_device.extension_names.num_strings,
		.ppEnabledExtensionNames = physical_device.extension_names.strings,
		.pEnabledFeatures = NULL
	};

	// Compatibility
	if (debug_enabled) {
		if (!check_device_validation_layer_support(physical_device.handle, vulkan_instance.layer_names)) {
			log_message(ERROR, "Required validation layers not supported by device.");
			create_info.enabledLayerCount = 0;
			create_info.ppEnabledLayerNames = NULL;
		}
		else {
			create_info.enabledLayerCount = vulkan_instance.layer_names.num_strings;
			create_info.ppEnabledLayerNames = vulkan_instance.layer_names.strings;
		}
	}
	else {
		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = NULL;
	}

	VkResult result = vkCreateDevice(physical_device.handle, &create_info, NULL, device_ptr);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Logical device creation failed. (Error code: %i)", result);
		exit(1); // TODO - better error handling
	}

	free(queue_create_infos);
}
