#ifndef FRAME_H
#define FRAME_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "CommandBuffer.h"
#include "descriptor.h"
#include "physical_device.h"
#include "synchronization.h"

// The frame is a set of resources used in drawing a frame.
// They are placed in a struct for easy duplication, so that multiple frames can be pipelined.

typedef struct Frame {
	
	CommandBuffer commandBuffer;
	
	DescriptorSet descriptorSet;

	// Signaled when the image for this frame is available.
	binary_semaphore_t semaphore_image_available;

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

	VkBuffer vertex_buffer;
	VkBuffer index_buffer;

} Frame;

typedef struct FrameArray {

	uint32_t current_frame;
	uint32_t num_frames;
	Frame *frames;

	VkDeviceMemory buffer_memory;
	VkDevice device;

} FrameArray;

typedef struct FrameArrayCreateInfo {
	
	uint32_t num_frames;
	PhysicalDevice physical_device;
	VkDevice vkDevice;
	
	CommandPool commandPool;
	DescriptorPool descriptorPool;
	
} FrameArrayCreateInfo;

FrameArray createFrameArray(const FrameArrayCreateInfo frameArrayCreateInfo);

bool deleteFrameArray(FrameArray *const frame_array_ptr);

#endif	// FRAME_H
