#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#include <vulkan/vulkan.h>

typedef struct memory_range_t {
	VkDeviceSize offset;
	VkDeviceSize size;
} memory_range_t;

typedef uint32_t memory_type_index_t;

// The memory type set provides a useful abstraction on the array of memory types available on the physical device.
// Memory types are automatically assigned to each named memory type index based on specific criteria,
// 	allowing for easy memory type selection throughout the operation of the application.
// This also effectively caches the memory types that will be most commonly used by the application.
typedef struct memory_type_set_t {

	// This memory type is to be used for resources read by the GPU each frame for drawing.
	// Ideally, this will be device-local but not host-visible.
	memory_type_index_t graphics_resources;

	// This memory type is to be used for buffers used for staging data to the GPU.
	// Ideally, this will be host-visible, but neither host-cached nor device-local.
	memory_type_index_t resource_staging;

	// This memory type is to be used for small, reusable uniform/storage buffers that are updated frequently.
	// Ideally, this will be both device-local and host-visible.
	memory_type_index_t uniform_data;

} memory_type_set_t;

// TODO - create dedicated function that finds the best memory type index for the graphics resources.
memory_type_set_t select_memory_types(VkPhysicalDevice physical_device);

VkDeviceSize align_offset(const VkDeviceSize offset, const VkDeviceSize alignment);

#endif	// MEMORY_H
