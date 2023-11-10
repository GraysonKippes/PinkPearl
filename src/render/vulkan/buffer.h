#ifndef BUFFER_H
#define BUFFER_H

#include <vulkan/vulkan.h>

#include "physical_device.h"

typedef struct buffer_t {
	
	VkBuffer m_handle;
	VkDeviceMemory m_memory;
	VkDeviceSize m_size;

} buffer_t;

// Always creates a GPU-side buffer. Use `stage_buffer_data` to upload data a buffer returned by this function.
buffer_t create_buffer(VkPhysicalDevice physical_device, VkDevice logical_device, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties);

buffer_t create_shared_buffer(physical_device_t physical_device, VkDevice logical_device, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties);

// Map some data to a host-visible buffer. This function allows setting offset into the buffer and size of data being transfer.
void map_data_to_buffer(VkDevice logical_device, buffer_t buffer, VkDeviceSize offset, VkDeviceSize size, void *data);

// Map some data to the entirety of a host-visible buffer. This function maps from the start of the buffer to the end.
void map_data_to_whole_buffer(VkDevice logical_device, buffer_t buffer, void *data);

void destroy_buffer(VkDevice logical_device, buffer_t buffer);

#endif	// BUFFER_H
