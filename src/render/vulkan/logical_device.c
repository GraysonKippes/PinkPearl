#include "logical_device.h"

#include <stdlib.h>

#include "debug.h"
#include "log/logging.h"

VkDeviceQueueCreateInfo make_queue_create_info(uint32_t queue_family_index) {
	
	static const float queue_priority = 1.0F;

	VkDeviceQueueCreateInfo queue_create_info;
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = queue_family_index;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &queue_priority;

	return queue_create_info;
}

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

static const char *validation_layers[] = {
	"VK_LAYER_KHRONOS_validation"
};

void create_logical_device(physical_device_t physical_device, VkDevice *logical_device_ptr) {

	log_message(INFO, "Creating logical device...");

	uint32_t num_queue_create_infos = 0;
	VkDeviceQueueCreateInfo *queue_create_infos = make_queue_create_infos(physical_device.m_queue_family_indices, &num_queue_create_infos);

	VkPhysicalDeviceFeatures device_features = { 0 };
	device_features.samplerAnisotropy = VK_TRUE;
	
	VkDeviceCreateInfo create_info;
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
		create_info.enabledLayerCount = 1;
		create_info.ppEnabledLayerNames = validation_layers;
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

void submit_graphics_queue(VkQueue queue, VkCommandBuffer *command_buffer_ptr, VkSemaphore *wait_semaphore_ptr, VkSemaphore *signal_semaphore, VkFence in_flight_fence) {

	VkSubmitInfo submit_info;
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
