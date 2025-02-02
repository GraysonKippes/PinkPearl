#ifndef FRAME_H
#define FRAME_H

#include <vulkan/vulkan.h>
#include "buffer.h"
#include "CommandBuffer.h"
#include "descriptor.h"
#include "physical_device.h"
#include "synchronization.h"

// TODO - switch to a data-oriented design
// 		- allow arbitrary creation of resources in array

// The frame is a set of resources used in drawing a frame.
// They are placed in a struct for easy duplication, so that multiple frames can be pipelined.

typedef struct Frame {

	// Signaled when the image for this frame is available.
	BinarySemaphore semaphore_image_available;

	// Signaled when this frame is done being rendered and can be displayed to the surface.
	BinarySemaphore semaphore_present_ready;
	TimelineSemaphore semaphore_render_finished;

	// Signaled when this frame is done being presented.
	VkFence fence_frame_ready;

	// Signaled when the buffers are fully updated after a transfer operation.
	// Unsignaled when there is a pending transfer operation;
	// 	this can be either when a request is put in to update the buffer data, 
	// 	OR when the request is being currently fulfilled.
	TimelineSemaphore semaphore_buffers_ready;

	VkBuffer vertex_buffer;
	VkBuffer index_buffer;

} Frame;

typedef struct FrameArray {

	uint32_t current_frame;
	uint32_t num_frames;
	Frame *frames;
	
	CmdBufArray cmdBufArray;

	VkDeviceMemory buffer_memory;
	VkDevice device;

} FrameArray;

typedef struct FrameArrayCreateInfo {
	
	uint32_t num_frames;
	PhysicalDevice physical_device;
	VkDevice vkDevice;
	
	CommandPool commandPool;
	
} FrameArrayCreateInfo;

FrameArray createFrameArray(const FrameArrayCreateInfo frameArrayCreateInfo);

bool deleteFrameArray(FrameArray *const pFrameArray);

#endif	// FRAME_H
