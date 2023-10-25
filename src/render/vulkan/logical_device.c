#include "logical_device.h"

#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "log/logging.h"

#include "layer.h"

bool check_device_validation_layer_support(VkPhysicalDevice physical_device, uint32_t num_required_layers, const char *required_layers[]);

// Returns a pointer-array that must be freed by the caller.
VkDeviceQueueCreateInfo *make_queue_create_infos(queue_family_indices_t queue_family_indices, uint32_t *num_queue_create_infos) {

	// Pack the queue family indices into an array for easy iteration.
	uint32_t indices_arr[NUM_QUEUES];
	indices_arr[0] = *queue_family_indices.m_graphics_family_ptr;
	indices_arr[1] = *queue_family_indices.m_present_family_ptr;
	indices_arr[2] = *queue_family_indices.m_transfer_family_ptr;
	indices_arr[3] = *queue_family_indices.m_compute_family_ptr;

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

void create_logical_device(physical_device_t physical_device, VkDevice *logical_device_ptr) {

	log_message(INFO, "Creating logical device...");

	uint32_t num_queue_create_infos = 0;
	VkDeviceQueueCreateInfo *queue_create_infos = make_queue_create_infos(physical_device.m_queue_family_indices, &num_queue_create_infos);

	VkPhysicalDeviceFeatures device_features = { 0 };
	device_features.samplerAnisotropy = VK_TRUE;
	
	VkDeviceCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.queueCreateInfoCount = num_queue_create_infos;
	create_info.pQueueCreateInfos = queue_create_infos;
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = physical_device.m_num_extensions;
	create_info.ppEnabledExtensionNames = physical_device.m_extensions;

	// Compatibility
	if (debug_enabled) {
		if (!check_device_validation_layer_support(physical_device.m_handle, num_validation_layers, validation_layers)) {
			log_message(ERROR, "Required validation layers not supported by device.");
			create_info.enabledLayerCount = 0;
			create_info.ppEnabledLayerNames = NULL;
		}
		else {
			create_info.enabledLayerCount = num_validation_layers;
			create_info.ppEnabledLayerNames = validation_layers;
		}
	}
	else {
		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = NULL;
	}

	VkResult result = vkCreateDevice(physical_device.m_handle, &create_info, NULL, logical_device_ptr);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Logical device creation failed. (Error code: %i)", result);
		exit(1);
	}

	free(queue_create_infos);
}

bool check_device_validation_layer_support(VkPhysicalDevice physical_device, uint32_t num_required_layers, const char *required_layers[]) {

	log_message(VERBOSE, "Checking device validation layer support...");

	for (size_t i = 0; i < num_required_layers; ++i) {
		logf_message(VERBOSE, "Required layer: \"%s\"", required_layers[i]);
	}

	uint32_t num_available_layers = 0;
	vkEnumerateDeviceLayerProperties(physical_device, &num_available_layers, NULL);

	if (num_available_layers < num_required_layers)
		return false;

	if (num_available_layers == 0) {
		if (num_required_layers == 0)
			return true;
		else
			return false;
	}

	VkLayerProperties *available_layers = calloc(num_available_layers, sizeof(VkLayerProperties));
	vkEnumerateDeviceLayerProperties(physical_device, &num_available_layers, available_layers);

	for (size_t i = 0; i < num_available_layers; ++i) {
		logf_message(VERBOSE, "Available layer: \"%s\"", available_layers[i].layerName);
	}

	for (size_t i = 0; i < num_required_layers; ++i) {

		const char *layer_name = required_layers[i];
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
	available_layers = NULL;

	return true;
}

void submit_graphics_queue(VkQueue queue, VkCommandBuffer *command_buffer_ptr, VkSemaphore *wait_semaphore_ptr, VkSemaphore *signal_semaphore, VkFence in_flight_fence) {

	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = command_buffer_ptr;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphore_ptr;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphore;

	if (vkQueueSubmit(queue, 1, &submit_info, in_flight_fence) != VK_SUCCESS) {
		// TODO - error handling
	}
}
