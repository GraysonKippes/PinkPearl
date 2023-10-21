#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "physical_device.h"

typedef struct buffer_t {
	
	VkBuffer m_handle;
	VkDeviceMemory m_memory;
	VkDeviceSize m_size;

} buffer_t;

// Type-definition for the integer type used for index data.
typedef uint16_t index_t;

// Always creates a GPU-side buffer. Use `stage_buffer_data` to upload data a buffer returned by this function.
buffer_t create_buffer(VkPhysicalDevice physical_device, VkDevice logical_device, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties);

// Creates a staging buffer with the data and uploads it to the specified buffer.
void stage_buffer_data(buffer_t buffer, size_t size, void *data_ptr);

void destroy_buffer(VkDevice logical_device, buffer_t buffer);

#endif	// BUFFER_H
