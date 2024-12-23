#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include <stdint.h>
#include <vulkan/vulkan.h>

/* -- BINARY SEMAPHORE -- */

typedef struct BinarySemaphore {
	VkSemaphore semaphore;
	VkDevice device;
} BinarySemaphore;

BinarySemaphore create_binary_semaphore(const VkDevice vk_device);
bool destroy_binary_semaphore(BinarySemaphore *const binary_semaphore_ptr);

VkSemaphoreSubmitInfo make_binary_semaphore_wait_submit_info(const BinarySemaphore binary_semaphore, const VkPipelineStageFlags2 stage_mask);
VkSemaphoreSubmitInfo make_binary_semaphore_signal_submit_info(const BinarySemaphore binary_semaphore, const VkPipelineStageFlags2 stage_mask);

/* -- TIMELINE SEMAPHORE -- */

typedef struct TimelineSemaphore {

	VkSemaphore semaphore;
	uint64_t wait_counter;

	VkDevice device;

} TimelineSemaphore;

TimelineSemaphore create_timeline_semaphore(const VkDevice vkDevice);
bool destroy_timeline_semaphore(TimelineSemaphore *const pSemaphore);

VkSemaphoreSubmitInfo make_timeline_semaphore_wait_submit_info(const TimelineSemaphore timeline_semaphore, const VkPipelineStageFlags2 stage_mask);
VkSemaphoreSubmitInfo make_timeline_semaphore_signal_submit_info(const TimelineSemaphore timeline_semaphore, const VkPipelineStageFlags2 stage_mask);

void timeline_to_binary_semaphore_signal(const VkQueue queue, const TimelineSemaphore timeline_semaphore, const BinarySemaphore binary_semaphore);

#endif	// SYNCHRONIZATION_H
