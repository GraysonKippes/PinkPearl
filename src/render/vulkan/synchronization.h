#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>



typedef struct binary_semaphore_t {
	VkSemaphore semaphore;
	VkDevice device;
} binary_semaphore_t;

binary_semaphore_t create_binary_semaphore(const VkDevice vk_device);
bool destroy_binary_semaphore(binary_semaphore_t *const binary_semaphore_ptr);

VkSemaphoreSubmitInfo make_binary_semaphore_wait_submit_info(const binary_semaphore_t binary_semaphore, const VkPipelineStageFlags2 stage_mask);
VkSemaphoreSubmitInfo make_binary_semaphore_signal_submit_info(const binary_semaphore_t binary_semaphore, const VkPipelineStageFlags2 stage_mask);



typedef struct timeline_semaphore_t {

	VkSemaphore semaphore;
	uint64_t wait_counter;

	VkDevice device;

} timeline_semaphore_t;

timeline_semaphore_t create_timeline_semaphore(const VkDevice vk_device);
bool destroy_timeline_semaphore(timeline_semaphore_t *const timeline_semaphore_ptr);

VkSemaphoreSubmitInfo make_timeline_semaphore_wait_submit_info(const timeline_semaphore_t timeline_semaphore, const VkPipelineStageFlags2 stage_mask);
VkSemaphoreSubmitInfo make_timeline_semaphore_signal_submit_info(const timeline_semaphore_t timeline_semaphore, const VkPipelineStageFlags2 stage_mask);

void timeline_to_binary_semaphore_signal(const VkQueue queue, const timeline_semaphore_t timeline_semaphore, const binary_semaphore_t binary_semaphore);


#endif	// SYNCHRONIZATION_H
