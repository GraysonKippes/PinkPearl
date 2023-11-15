#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "image.h"

// The memory type set provides a useful abstraction on the array of memory types available on the physical device.
// Memory types are automatically assigned to each named memory type index based on specific criteria,
// 	allowing for easy memory type selection throughout the operation of the application.
// This also effectively caches the memory types that will be most commonly used by the application.

typedef struct memory_type_set_t {

	// This memory type is to be used for resources read by the GPU each frame for drawing.
	// Ideally, this will be device-local but not host-visible.
	uint32_t m_graphics_resources;

	// This memory type is to be used for buffers used for staging data to the GPU.
	// Ideally, this will be host-visible, but neither host-cached nor device-local.
	uint32_t m_resource_staging;

	// This memory type is to be used for small, reusable uniform/storage buffers that are updated frequently.
	// Ideally, this will be both device-local and host-visible.
	uint32_t m_uniform_data;

} memory_type_set_t;

memory_type_set_t select_memory_types(VkPhysicalDevice physical_device);

// TODO - create dedicated function that finds the best memory type index for the graphics resources.

void allocate_device_memory(VkDevice device, VkDeviceSize size, uint32_t memory_type, VkDeviceMemory *memory_ptr);

void bind_buffer_to_memory(buffer_t buffer, VkDeviceMemory memory, VkDeviceSize offset);

#endif	// MEMORY_H
