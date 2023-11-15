#ifndef BUFFER_H
#define BUFFER_H

#include <vulkan/vulkan.h>

#include "physical_device.h"
#include "queue.h"

typedef struct buffer_t {
	
	VkBuffer handle;
	VkDeviceMemory memory;
	VkDeviceSize size;

	VkDevice device;

} buffer_t;

// Always creates a GPU-side buffer. Use `stage_buffer_data` to upload data a buffer returned by this function.
buffer_t create_buffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties, queue_family_set_t queue_family_set);

void destroy_buffer(buffer_t *buffer_ptr);

VkMemoryRequirements get_buffer_memory_requirements(buffer_t buffer);

// Map some data to a host-visible buffer. This function allows setting offset into the buffer and size of data being transfer.
void map_data_to_buffer(VkDevice device, buffer_t buffer, VkDeviceSize offset, VkDeviceSize size, void *data);

// Map some data to the entirety of a host-visible buffer. This function maps from the start of the buffer to the end.
void map_data_to_whole_buffer(VkDevice device, buffer_t buffer, void *data);

#endif	// BUFFER_H
