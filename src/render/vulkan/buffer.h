#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "util/byte.h"

#include "memory.h"
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

// TODO - get rid of this.
typedef struct staging_buffer_t {

	VkBuffer handle;
	VkDeviceMemory memory;
	VkDeviceSize size;

	void *mapped_memory;

	VkDevice device;

} staging_buffer_t;

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

VkDeviceMemory allocate_device_memory(const VkDevice device, const VkDeviceSize size, const memory_type_index_t memory_type_index);
void bind_buffer_to_memory(buffer_t buffer, VkDeviceMemory memory, VkDeviceSize offset);

VkMemoryRequirements get_buffer_memory_requirements(buffer_t buffer);

// Map some data to a host-visible buffer. This function allows setting offset into the buffer and size of data being transfer.
void map_data_to_buffer(VkDevice device, buffer_t buffer, VkDeviceSize offset, VkDeviceSize size, void *data);

// Map some data to the entirety of a host-visible buffer. This function maps from the start of the buffer to the end.
void map_data_to_whole_buffer(VkDevice device, buffer_t buffer, void *data);

#endif	// BUFFER_H
