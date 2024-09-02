#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#include <vulkan/vulkan.h>

typedef struct MemoryRange {
	VkDeviceSize offset;
	VkDeviceSize size;
} MemoryRange;

// The memory type set provides a useful abstraction on the array of memory types available on the physical device.
// Memory types are automatically assigned to each named memory type index based on specific criteria,
// 	allowing for easy memory type selection throughout the operation of the application.
// This also effectively caches the memory types that will be most commonly used by the application.
typedef struct MemoryTypeIndexSet {

	// This memory type is to be used for resources read by the GPU each frame for drawing.
	// Ideally, this will be device-local but not host-visible.
	uint32_t graphics_resources;

	// This memory type is to be used for buffers used for staging data to the GPU.
	// Ideally, this will be host-visible, but neither host-cached nor device-local.
	uint32_t resource_staging;

	// This memory type is to be used for small, reusable uniform/storage buffers that are updated frequently.
	// Ideally, this will be both device-local and host-visible.
	uint32_t uniform_data;

} MemoryTypeIndexSet;

// TODO - create dedicated function that finds the best memory type index for the graphics resources.
MemoryTypeIndexSet select_memory_types(VkPhysicalDevice physical_device);

VkDeviceSize alignOffset(const VkDeviceSize offset, const VkDeviceSize alignment);

#endif	// MEMORY_H
