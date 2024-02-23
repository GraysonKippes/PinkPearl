#ifndef FRAME_H
#define FRAME_H

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "descriptor.h"
#include "physical_device.h"

// The frame is a set of resources used in drawing a frame.
// They are placed in a struct for easy duplication, so that multiple frames can be pipelined.

typedef struct frame_t {

	// Buffer to upload drawing commands for this frame to.
	VkCommandBuffer command_buffer;

	VkDescriptorSet descriptor_set;

	/* -- Synchronization -- */

	// Signaled when the image for this frame is available.
	VkSemaphore semaphore_image_available;

	// Signaled when this frame is done being rendered and can be displayed to the surface.
	VkSemaphore semaphore_render_finished;

	// Signaled when this frame is done being presented.
	VkFence fence_frame_ready;

	// Signaled when the buffers are fully updated after a transfer operation.
	// Unsignaled when there is a pending transfer operation;
	// 	this can be either when a request is put in to update the buffer data, OR when the request is being currently fulfilled.
	VkFence fence_buffers_up_to_date;

	// Each bit indicates if a slot in the model buffer needs to be updated.
	uint64_t model_update_flags;

	/* -- Buffers -- */

	// This is the model buffer for the entire scene.
	// It is (currently) divided into three partitions: the room partition and the entity partition.
	//
	// The room partition has enough space to store the models of two rooms simultaneously.
	// When the player moves to another room, the new room's model is loaded into the slot not currently being used in the room partition,
	// and a flag is set indicating that the current room's model is now stored in the other slot.
	// This allows for two rooms to be rendered at once in the room transition, and it also effectively caches the model for the previous room.
	// If the player goes back into the old room, then no model has to loaded, as the data already exists in the buffer.
	//
	// The entity partition has enough space to store 64 standard rects.
	// In the future, an additional partition may be added for larger models.
	// Render handles are used to index into this partition.
	//
	buffer_t model_buffer;

	buffer_t index_buffer;

} frame_t;

frame_t create_frame(physical_device_t physical_device, VkDevice device, VkCommandPool command_pool, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout);

void destroy_frame(VkDevice device, frame_t frame);

typedef struct frame_array_create_info_t {

	VkPhysicalDevice physical_device;
	VkDevice device;
	uint32_t num_frames;

} frame_array_create_info_t;

typedef struct frame_array_t {

	uint32_t num_frames;
	frame_t *frames;

	VkDeviceMemory buffer_memory;
	VkDevice device;

} frame_array_t;

#endif	// FRAME_H
