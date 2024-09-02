#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "util/Types.h"

#include "memory.h"
#include "physical_device.h"
#include "queue.h"

typedef enum BufferType {
	BUFFER_TYPE_STAGING,
	BUFFER_TYPE_UNIFORM,
	BUFFER_TYPE_STORAGE,
	BUFFER_TYPE_DRAW_DATA
} BufferType;

typedef struct BufferPartitionCreateInfo {

	VkPhysicalDevice physical_device;
	VkDevice device;

	BufferType buffer_type;
	MemoryTypeIndexSet memory_type_set;

	uint32_t num_queue_family_indices;
	uint32_t *queue_family_indices;

	uint32_t num_partition_sizes;
	VkDeviceSize *partition_sizes;

} BufferPartitionCreateInfo;

// Represents a partitioning of a buffer into sub-resources.
typedef struct BufferPartition {

	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize total_memory_size;

	uint32_t num_ranges;
	MemoryRange *ranges;

	VkDevice device;
	
} BufferPartition;

BufferPartition create_buffer_partition(const BufferPartitionCreateInfo buffer_partition_create_info);
bool destroy_buffer_partition(BufferPartition *const buffer_partition_ptr);

byte_t *buffer_partition_map_memory(const BufferPartition buffer_partition, const uint32_t partition_index);
void buffer_partition_unmap_memory(const BufferPartition buffer_partition);

VkDescriptorBufferInfo buffer_partition_descriptor_info(const BufferPartition buffer_partition, const uint32_t partition_index);

#endif	// BUFFER_H
