#ifndef FRAME_H
#define FRAME_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "descriptor.h"
#include "physical_device.h"
#include "synchronization.h"

// The frame is a set of resources used in drawing a frame.
// They are placed in a struct for easy duplication, so that multiple frames can be pipelined.

typedef struct frame_t {

	VkCommandBuffer command_buffer;
	VkDescriptorSet descriptor_set;

	/* -- Synchronization -- */

	// Signaled when the image for this frame is available.
	// TODO - use binary_semaphore_t struct.
	VkSemaphore semaphore_image_available;

	// Signaled when this frame is done being rendered and can be displayed to the surface.
	binary_semaphore_t semaphore_present_ready;
	timeline_semaphore_t semaphore_render_finished;

	// Signaled when this frame is done being presented.
	VkFence fence_frame_ready;

	// Signaled when the buffers are fully updated after a transfer operation.
	// Unsignaled when there is a pending transfer operation;
	// 	this can be either when a request is put in to update the buffer data, 
	// 	OR when the request is being currently fulfilled.
	timeline_semaphore_t semaphore_buffers_ready;

	/* -- Buffers -- */

	VkBuffer vertex_buffer;
	VkBuffer index_buffer;

} frame_t;

typedef struct frame_array_t {

	uint32_t current_frame;
	uint32_t num_frames;
	frame_t *frames;

	VkDeviceMemory buffer_memory;
	VkDevice device;

} frame_array_t;

typedef struct frame_array_create_info_t {
	uint32_t num_frames;
	physical_device_t physical_device;
	VkDevice device;
	VkCommandPool command_pool;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
} frame_array_create_info_t;

frame_array_t create_frame_array(const frame_array_create_info_t frame_array_create_info);

bool destroy_frame_array(frame_array_t *const frame_array_ptr);

#endif	// FRAME_H
