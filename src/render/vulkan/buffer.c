#include "buffer.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "log/logging.h"
#include "util/allocate.h"

#include "command_buffer.h"

const VkBufferUsageFlags buffer_usage_vertex = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
const VkBufferUsageFlags buffer_usage_index = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

const VkMemoryPropertyFlags memory_properties_staging = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
const VkMemoryPropertyFlags memory_properties_local = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

buffer_t create_buffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties, queue_family_set_t queue_family_set) {

	buffer_t buffer = { 0 };

	buffer.size = size;
	buffer.device = device;

	VkBufferCreateInfo buffer_create_info = { 0 };
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = NULL;
	buffer_create_info.flags = 0;
	buffer_create_info.size = buffer.size;
	buffer_create_info.usage = buffer_usage;

	if (is_queue_family_set_null(queue_family_set)) {
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buffer_create_info.queueFamilyIndexCount = 0;
		buffer_create_info.pQueueFamilyIndices = NULL;
	}
	else {
		buffer_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		buffer_create_info.queueFamilyIndexCount = queue_family_set.num_queue_families;
		buffer_create_info.pQueueFamilyIndices = queue_family_set.queue_families;
	}
	
	// TODO - error handling
	VkResult result = vkCreateBuffer(buffer.device, &buffer_create_info, NULL, &buffer.handle);
	if (result != VK_SUCCESS) {
		logf_message(ERROR, "Buffer creation failed. (Error code: %i)", result);
		return buffer;
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(buffer.device, buffer.handle, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = memory_requirements.size;
	if(!find_physical_device_memory_type(physical_device, memory_requirements.memoryTypeBits, memory_properties, &allocate_info.memoryTypeIndex)) {
		// TODO - error handling
	}

	// TODO - error handling
	vkAllocateMemory(buffer.device, &allocate_info, NULL, &buffer.memory);
	vkBindBufferMemory(buffer.device, buffer.handle, buffer.memory, 0);

	return buffer;
}

void destroy_buffer(buffer_t *buffer_ptr) {

	if (buffer_ptr == NULL) {
		return;
	}

	vkDestroyBuffer(buffer_ptr->device, buffer_ptr->handle, NULL);
	vkFreeMemory(buffer_ptr->device, buffer_ptr->memory, NULL);

	buffer_ptr->handle = VK_NULL_HANDLE;
	buffer_ptr->memory = VK_NULL_HANDLE;
	buffer_ptr->size = 0;
	buffer_ptr->device = VK_NULL_HANDLE;
}

VkMemoryRequirements get_buffer_memory_requirements(buffer_t buffer) {

	VkMemoryRequirements memory_requirements = { 0 };
	vkGetBufferMemoryRequirements(buffer.device, buffer.handle, &memory_requirements);

	return memory_requirements;
}

buffer_partition_t create_buffer_partition(const buffer_partition_create_info_t buffer_partition_create_info) {
	
	buffer_partition_t buffer_partition = {
		.buffer = VK_NULL_HANDLE,
		.memory = VK_NULL_HANDLE,
		.total_memory_size = 0,
		.num_ranges = 0,
		.ranges = NULL,
		.device = VK_NULL_HANDLE
	};

	if (buffer_partition_create_info.partition_sizes == NULL) {
		log_message(ERROR, "Error creating buffer partition: memory ranges pointer-array is NULL.");
		return buffer_partition;
	}

	buffer_partition.num_ranges = buffer_partition_create_info.num_partition_sizes;
	if (!allocate((void **)&buffer_partition.ranges, buffer_partition.num_ranges, sizeof(memory_range_t))) {
		log_message(ERROR, "Error creating buffer partition: failed to allocate partition ranges pointer-array.");
		buffer_partition.num_ranges = 0;
		return buffer_partition;
	}

	buffer_partition.device = buffer_partition_create_info.device;

	VkPhysicalDeviceProperties2 physical_device_properties = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		.pNext = NULL,
		.properties = { 0 }
	};
	
	vkGetPhysicalDeviceProperties2(buffer_partition_create_info.physical_device, &physical_device_properties);

	// TODO - add a check for the buffer type (i.e. uniform, storage, texel, etc).
	const VkDeviceSize offset_alignment = physical_device_properties.properties.limits.minUniformBufferOffsetAlignment;
	for (uint32_t i = 0; i < buffer_partition.num_ranges; ++i) {
		const VkDeviceSize partition_size = buffer_partition_create_info.partition_sizes[i];
		buffer_partition.ranges[i].offset = buffer_partition.total_memory_size;
		buffer_partition.ranges[i].size = partition_size;
		buffer_partition.total_memory_size += align_offset(partition_size, offset_alignment);
	}

	VkBufferCreateInfo buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.size = buffer_partition.total_memory_size,
		.usage = buffer_partition_create_info.buffer_usage_flags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL
	};

	if (buffer_partition_create_info.num_queue_family_indices > 0 && buffer_partition_create_info.queue_family_indices != NULL) {
		buffer_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		buffer_create_info.queueFamilyIndexCount = buffer_partition_create_info.num_queue_family_indices;
		buffer_create_info.pQueueFamilyIndices = buffer_partition_create_info.queue_family_indices;
	}

	const VkResult buffer_create_result = vkCreateBuffer(buffer_partition.device, &buffer_create_info, NULL, &buffer_partition.buffer);
	if (buffer_create_result < 0) {
		logf_message(ERROR, "Error creating buffer partition: buffer creation failed (result code: %i).", buffer_create_result);
		destroy_buffer_partition(&buffer_partition);
		return buffer_partition;
	}
	else if (buffer_create_result > 0) {
		logf_message(WARNING, "Warning creating buffer partition: buffer creation returned with warning (result code: %i).", buffer_create_result);
	}

	const VkBufferMemoryRequirementsInfo2 memory_requirements_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
		.pNext = NULL,
		.buffer = buffer_partition.buffer
	};

	VkMemoryRequirements2 memory_requirements = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
		.pNext = NULL,
		.memoryRequirements = (VkMemoryRequirements){
			.size = 0,
			.alignment = 0,
			.memoryTypeBits = 0
		}
	};

	vkGetBufferMemoryRequirements2(buffer_partition.device, &memory_requirements_info, &memory_requirements);

	const VkMemoryAllocateInfo memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = memory_requirements.memoryRequirements.size,
		.memoryTypeIndex = buffer_partition_create_info.memory_type_index
	};

	const VkResult memory_allocate_result = vkAllocateMemory(buffer_partition.device, &memory_allocate_info, NULL, &buffer_partition.memory);
	if (memory_allocate_result < 0) {
		logf_message(ERROR, "Error creating buffer partition: memory allocation failed (result code: %i).", memory_allocate_result);
		destroy_buffer_partition(&buffer_partition);
		return buffer_partition;
	}
	else if (memory_allocate_result > 0) {
		logf_message(WARNING, "Warning creating buffer partition: memory allocation returned with warning (result code: %i).", memory_allocate_result);
	}

	const VkBindBufferMemoryInfo bind_buffer_memory_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
		.pNext = NULL,
		.buffer = buffer_partition.buffer,
		.memory = buffer_partition.memory,
		.memoryOffset = 0
	};

	const VkResult bind_buffer_memory_result = vkBindBufferMemory2(buffer_partition.device, 1, &bind_buffer_memory_info);
	if (bind_buffer_memory_result < 0) {
		logf_message(ERROR, "Error creating buffer partition: memory binding failed (result code: %i).", bind_buffer_memory_result);
		destroy_buffer_partition(&buffer_partition);
		return buffer_partition;
	}
	else if (bind_buffer_memory_result > 0) {
		logf_message(WARNING, "Warning creating buffer partition: memory binding returned with warning (result code: %i).", bind_buffer_memory_result);
	}

	return buffer_partition;
}

bool destroy_buffer_partition(buffer_partition_t *const buffer_partition_ptr) {

	if (buffer_partition_ptr == NULL) {
		return false;
	}

	vkDestroyBuffer(buffer_partition_ptr->device, buffer_partition_ptr->buffer, NULL);
	buffer_partition_ptr->buffer = VK_NULL_HANDLE;

	vkFreeMemory(buffer_partition_ptr->device, buffer_partition_ptr->memory, NULL);
	buffer_partition_ptr->memory = VK_NULL_HANDLE;

	free(buffer_partition_ptr->ranges);
	buffer_partition_ptr->ranges = NULL;
	buffer_partition_ptr->total_memory_size = 0;
	buffer_partition_ptr->num_ranges = 0;

	buffer_partition_ptr->device = VK_NULL_HANDLE;

	return true;
}

byte_t *buffer_partition_map_memory(const buffer_partition_t buffer_partition, const uint32_t partition_index) {

	if (partition_index >= buffer_partition.num_ranges) {
		logf_message(ERROR, "Error mapping buffer partition memory: partition index (%u) is not less than number of partition ranges (%u).", partition_index, buffer_partition.num_ranges);
		return NULL;
	}

	byte_t *mapped_memory = NULL;
	const VkResult map_memory_result = vkMapMemory(buffer_partition.device, buffer_partition.memory, 
			buffer_partition.ranges[partition_index].offset,
			buffer_partition.ranges[partition_index].size,
			0, (void **)&mapped_memory);

	if (map_memory_result < 0) {
		logf_message(ERROR, "Error mapping buffer partition memory: memory mapping failed (result code: %i).", map_memory_result);
		return NULL;
	}
	else if (map_memory_result > 0) {
		logf_message(WARNING, "Warning mapping buffer partition memory: memory mapping returned with warning (result code: %i).", map_memory_result);
	}

	return mapped_memory;
}

void buffer_partition_unmap_memory(const buffer_partition_t buffer_partition) {
	vkUnmapMemory(buffer_partition.device, buffer_partition.memory);
}

VkDescriptorBufferInfo buffer_partition_descriptor_info(const buffer_partition_t buffer_partition, const uint32_t partition_index) {

	if (partition_index >= buffer_partition.num_ranges) {
		logf_message(ERROR, "Error making buffer partition descriptor info: partition index (%u) is not less than number of partition ranges (%u).", partition_index, buffer_partition.num_ranges);
		return (VkDescriptorBufferInfo){
			.buffer = VK_NULL_HANDLE,
			.offset = 0,
			.range = 0
		};
	}

	return (VkDescriptorBufferInfo){
		.buffer = buffer_partition.buffer,
		.offset = buffer_partition.ranges[partition_index].offset,
		.range = buffer_partition.ranges[partition_index].size
	};
}

VkDeviceMemory allocate_device_memory(const VkDevice device, const VkDeviceSize size, const memory_type_index_t memory_type_index) {
	
	const VkMemoryAllocateInfo allocate_info = { 
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = size,
		.memoryTypeIndex = memory_type_index
	};

	VkDeviceMemory device_memory = VK_NULL_HANDLE;
	vkAllocateMemory(device, &allocate_info, NULL, &device_memory);
	return device_memory;
}

void bind_buffer_to_memory(buffer_t buffer, VkDeviceMemory memory, VkDeviceSize offset) {
	vkBindBufferMemory(buffer.device, buffer.handle, memory, offset);
}

void map_data_to_buffer(VkDevice device, buffer_t buffer, VkDeviceSize offset, VkDeviceSize size, void *data) {
	void *mapped_data;
	vkMapMemory(device, buffer.memory, offset, size, 0, &mapped_data);
	memcpy(mapped_data, data, size);
	vkUnmapMemory(device, buffer.memory);
}

void map_data_to_whole_buffer(VkDevice device, buffer_t buffer, void *data) {
	void *mapped_data;
	vkMapMemory(device, buffer.memory, 0, buffer.size, 0, &mapped_data);
	memcpy(mapped_data, data, buffer.size);
	vkUnmapMemory(device, buffer.memory);
}
