#include "synchronization.h"

#include <stddef.h>

binary_semaphore_t create_binary_semaphore(const VkDevice vk_device) {

	binary_semaphore_t binary_semaphore = {
		.semaphore = VK_NULL_HANDLE,
		.device = vk_device
	};

	const VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0
	};

	vkCreateSemaphore(vk_device, &semaphore_create_info, nullptr, &binary_semaphore.semaphore);
	return binary_semaphore;
}

bool destroy_binary_semaphore(binary_semaphore_t *const binary_semaphore_ptr) {

	if (binary_semaphore_ptr == nullptr) {
		return false;
	}

	vkDestroySemaphore(binary_semaphore_ptr->device, binary_semaphore_ptr->semaphore, nullptr);
	binary_semaphore_ptr->semaphore = VK_NULL_HANDLE;
	binary_semaphore_ptr->device = VK_NULL_HANDLE;

	return true;
}

VkSemaphoreSubmitInfo make_binary_semaphore_wait_submit_info(const binary_semaphore_t binary_semaphore, const VkPipelineStageFlags2 stage_mask) {
	return (VkSemaphoreSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = binary_semaphore.semaphore,
		.value = 0,
		.stageMask = stage_mask,
		.deviceIndex = 0
	};
}

VkSemaphoreSubmitInfo make_binary_semaphore_signal_submit_info(const binary_semaphore_t binary_semaphore, const VkPipelineStageFlags2 stage_mask) {
	return (VkSemaphoreSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = binary_semaphore.semaphore,
		.value = 0,
		.stageMask = stage_mask,
		.deviceIndex = 0
	};
}

timeline_semaphore_t create_timeline_semaphore(const VkDevice vk_device) {

	timeline_semaphore_t timeline_semaphore = {
		.semaphore = VK_NULL_HANDLE,
		.wait_counter = 0,
		.device = vk_device
	};

	const VkSemaphoreTypeCreateInfo semaphore_type_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.pNext = nullptr,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = 0
	};

	const VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &semaphore_type_create_info,
		.flags = 0
	};

	vkCreateSemaphore(vk_device, &semaphore_create_info, nullptr, &timeline_semaphore.semaphore);
	return timeline_semaphore;
}

bool destroy_timeline_semaphore(timeline_semaphore_t *const timeline_semaphore_ptr) {

	if (timeline_semaphore_ptr == nullptr) {
		return false;
	}

	vkDestroySemaphore(timeline_semaphore_ptr->device, timeline_semaphore_ptr->semaphore, nullptr);
	timeline_semaphore_ptr->semaphore = VK_NULL_HANDLE;
	timeline_semaphore_ptr->wait_counter = 0;
	timeline_semaphore_ptr->device = VK_NULL_HANDLE;

	return true;
}

VkSemaphoreSubmitInfo make_timeline_semaphore_wait_submit_info(const timeline_semaphore_t timeline_semaphore, const VkPipelineStageFlags2 stage_mask) {
	return (VkSemaphoreSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = timeline_semaphore.semaphore,
		.value = timeline_semaphore.wait_counter,
		.stageMask = stage_mask,
		.deviceIndex = 0
	};
}

VkSemaphoreSubmitInfo make_timeline_semaphore_signal_submit_info(const timeline_semaphore_t timeline_semaphore, const VkPipelineStageFlags2 stage_mask) {
	return (VkSemaphoreSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = timeline_semaphore.semaphore,
		.value = timeline_semaphore.wait_counter + 1,
		.stageMask = stage_mask,
		.deviceIndex = 0
	};
}

void timeline_to_binary_semaphore_signal(const VkQueue queue, const timeline_semaphore_t timeline_semaphore, const binary_semaphore_t binary_semaphore) {

	VkSemaphoreSubmitInfo wait_semaphore_submit_info = make_timeline_semaphore_wait_submit_info(timeline_semaphore, VK_PIPELINE_STAGE_2_NONE);
	VkSemaphoreSubmitInfo signal_semaphore_submit_info = make_binary_semaphore_wait_submit_info(binary_semaphore, VK_PIPELINE_STAGE_2_NONE);

	const VkSubmitInfo2 submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.flags = 0,
		.waitSemaphoreInfoCount = 1,
		.pWaitSemaphoreInfos = &wait_semaphore_submit_info,
		.commandBufferInfoCount = 0,
		.pCommandBufferInfos = nullptr,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = &signal_semaphore_submit_info
	};

	vkQueueSubmit2(queue, 1, &submit_info, VK_NULL_HANDLE);
}
