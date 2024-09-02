#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "util/Types.h"

#include "memory.h"
#include "physical_device.h"
#include "queue.h"

typedef enum buffer_type_t {
	BUFFER_TYPE_STAGING,
	BUFFER_TYPE_UNIFORM,
	BUFFER_TYPE_STORAGE,
	BUFFER_TYPE_DRAW_DATA
} buffer_type_t;

typedef struct buffer_partition_create_info_t {

	VkPhysicalDevice physical_device;
	VkDevice device;

	buffer_type_t buffer_type;
	memory_type_set_t memory_type_set;

	uint32_t num_queue_family_indices;
	uint32_t *queue_family_indices;

	uint32_t num_partition_sizes;
	VkDeviceSize *partition_sizes;

} buffer_partition_create_info_t;

// Represents a partitioning of a buffer into sub-resources.
typedef struct buffer_partition_t {

	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize total_memory_size;

	uint32_t num_ranges;
	memory_range_t *ranges;

	VkDevice device;
	
} buffer_partition_t;

buffer_partition_t create_buffer_partition(const buffer_partition_create_info_t buffer_partition_create_info);
bool destroy_buffer_partition(buffer_partition_t *const buffer_partition_ptr);

byte_t *buffer_partition_map_memory(const buffer_partition_t buffer_partition, const uint32_t partition_index);
void buffer_partition_unmap_memory(const buffer_partition_t buffer_partition);

VkDescriptorBufferInfo buffer_partition_descriptor_info(const buffer_partition_t buffer_partition, const uint32_t partition_index);

#endif	// BUFFER_H
