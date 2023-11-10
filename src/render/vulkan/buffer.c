#include "buffer.h"

#include <stddef.h>
#include <string.h>

#include "command_buffer.h"

#include "log/logging.h"

const VkBufferUsageFlags buffer_usage_vertex = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

const VkBufferUsageFlags buffer_usage_index = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

const VkMemoryPropertyFlags memory_properties_staging = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

const VkMemoryPropertyFlags memory_properties_local = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

buffer_t create_buffer(VkPhysicalDevice physical_device, VkDevice logical_device, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties) {

	buffer_t buffer = { 0 };

	buffer.m_size = size;

	VkBufferCreateInfo buffer_info = { 0 };
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.pNext = NULL;
	buffer_info.flags = 0;
	buffer_info.size = buffer.m_size;
	buffer_info.usage = buffer_usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_info.queueFamilyIndexCount = 0;
	buffer_info.pQueueFamilyIndices = NULL;

	// TODO - error handling
	VkResult result = vkCreateBuffer(logical_device, &buffer_info, NULL, &buffer.m_handle);
	if (result != VK_SUCCESS) {
		logf_message(ERROR, "Buffer creation failed. (Error code: %i)", result);
		return buffer;
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(logical_device, buffer.m_handle, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = memory_requirements.size;
	if(!find_physical_device_memory_type(physical_device, memory_requirements.memoryTypeBits, memory_properties, &allocate_info.memoryTypeIndex)) {
		// TODO - error handling
	}

	// TODO - error handling
	vkAllocateMemory(logical_device, &allocate_info, NULL, &buffer.m_memory);

	vkBindBufferMemory(logical_device, buffer.m_handle, buffer.m_memory, 0);

	return buffer;
}

buffer_t create_shared_buffer(physical_device_t physical_device, VkDevice logical_device, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties) {

	buffer_t buffer = { 0 };

	buffer.m_size = size;

	uint32_t queue_family_indices[2] = { 
		*physical_device.m_queue_family_indices.m_graphics_family_ptr, 
		*physical_device.m_queue_family_indices.m_transfer_family_ptr 
	};

	VkBufferCreateInfo buffer_info = { 0 };
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.pNext = NULL;
	buffer_info.flags = 0;
	buffer_info.size = buffer.m_size;
	buffer_info.usage = buffer_usage;
	buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
	buffer_info.queueFamilyIndexCount = 2;
	buffer_info.pQueueFamilyIndices = queue_family_indices;

	// TODO - error handling
	vkCreateBuffer(logical_device, &buffer_info, NULL, &buffer.m_handle);

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(logical_device, buffer.m_handle, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = memory_requirements.size;
	if(!find_physical_device_memory_type(physical_device.m_handle, memory_requirements.memoryTypeBits, memory_properties, &allocate_info.memoryTypeIndex)) {
		// TODO - error handling
	}

	// TODO - error handling
	vkAllocateMemory(logical_device, &allocate_info, NULL, &buffer.m_memory);

	vkBindBufferMemory(logical_device, buffer.m_handle, buffer.m_memory, 0);

	return buffer;
}

void map_data_to_buffer(VkDevice logical_device, buffer_t buffer, VkDeviceSize offset, VkDeviceSize size, void *data) {
	void *mapped_data;
	vkMapMemory(logical_device, buffer.m_memory, offset, size, 0, &mapped_data);
	memcpy(mapped_data, data, size);
	vkUnmapMemory(logical_device, buffer.m_memory);
}

void map_data_to_whole_buffer(VkDevice logical_device, buffer_t buffer, void *data) {
	void *mapped_data;
	vkMapMemory(logical_device, buffer.m_memory, 0, buffer.m_size, 0, &mapped_data);
	memcpy(mapped_data, data, buffer.m_size);
	vkUnmapMemory(logical_device, buffer.m_memory);
}

void transfer_data_to_buffer(VkDevice logical_device, VkQueue queue, VkCommandPool command_pool, buffer_t source, buffer_t destination) {
	
	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;

	allocate_command_buffers(logical_device, command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vkEndCommandBuffer(transfer_command_buffer);
	submit_command_buffers_async(queue, 1, &transfer_command_buffer);
}

void destroy_buffer(VkDevice logical_device, buffer_t buffer) {
	vkDestroyBuffer(logical_device, buffer.m_handle, NULL);
	vkFreeMemory(logical_device, buffer.m_memory, NULL);
}
