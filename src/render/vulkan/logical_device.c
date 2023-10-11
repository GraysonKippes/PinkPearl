#include "logical_device.h"

VkDeviceQueueCreateInfo make_queue_create_info(uint32_t queue_family_index) {
	
	static const float queue_priority = 1.0F;

	VkDeviceQueueCreateInfo queue_create_info;
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = queue_family_index;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &queue_priority;

	return queue_create_info;
}

void create_logical_device(physical_device_t physical_device, VkDevice *logical_device_ptr) {

	static const uint32_t num_queues = 4;
	VkDeviceQueueCreateInfo queue_create_infos[num_queues];
	queue_create_infos[0] = make_queue_create_info(*physical_device.m_queue_family_indices.m_graphics_family_ptr);
	queue_create_infos[1] = make_queue_create_info(*physical_device.m_queue_family_indices.m_present_family_ptr);
	queue_create_infos[2] = make_queue_create_info(*physical_device.m_queue_family_indices.m_transfer_family_ptr);
	queue_create_infos[3] = make_queue_create_info(*physical_device.m_queue_family_indices.m_compute_family_ptr);

	VkPhysicalDeviceFeatures device_features;
	device_features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = num_queues;
	create_info.pQueueCreateInfos = queue_create_infos;
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = physical_device.m_num_extensions;
	create_info.ppEnabledExtensionNames = physical_device.m_extensions;

	if (vkCreateDevice(physical_device.m_handle, &create_info, NULL, logical_device_ptr) != VK_SUCCESS) {
		// TODO - error handling
	}
}

void create_command_pool(VkDevice logical_device, uint32_t queue_family_index, VkCommandPool *command_pool_ptr) {

	VkCommandPoolCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = queue_family_index;

	if (vkCreateCommandPool(logical_device, &create_info, NULL, command_pool_ptr) != VK_SUCCESS) {
		// TODO - error handling
	}
}

void create_transfer_command_pool(VkDevice logical_device, uint32_t queue_family_index, VkCommandPool *command_pool_ptr) {

	VkCommandPoolCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // TODO - this pool may not need the reset flag.
	create_info.queueFamilyIndex = queue_family_index;

	if (vkCreateCommandPool(logical_device, &create_info, NULL, command_pool_ptr) != VK_SUCCESS) {
		// TODO - error handling
	}
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
