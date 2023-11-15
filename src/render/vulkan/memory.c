#include "memory.h"

#include <stddef.h>

#include "log/logging.h"

memory_type_set_t select_memory_types(VkPhysicalDevice physical_device) {

	// TODO - redesign this whole function to be flexible with other types of GPUs.
	// 	Currently it only really targets NVIDIA/AMD GPUs.

	log_message(VERBOSE, "Selecting GPU memory types...");

	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	memory_type_set_t memory_type_set = { 0 };

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {

		VkMemoryPropertyFlags property_flags = memory_properties.memoryTypes[i].propertyFlags;

		// If the memory is HOST_CACHED, it is good for GPU-CPU data transfer, so DON'T use it, 
		// 	because Pink Pearl does not need GPU-CPU data transfer.
		if (property_flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
			continue;
		}

		// Main graphics resources memory type:
		// This memory type is to be used for the primary graphics resources (i.e. vertex buffer, index buffer).
		if ((property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) 
				&& !(property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
			memory_type_set.m_graphics_resources = i;
		}

		// Resource staging memory type:
		// This memory type is to be used for buffers that stage data to the GPU.
		if (!(property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) 
				&& (property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
			memory_type_set.m_resource_staging = i;
		}

		// Uniform buffer memory type:
		// This memory type is to be used for small reused uniform buffers that updated very often (i.e. on a per-frame basis).
		if ((property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) 
				&& (property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
			memory_type_set.m_uniform_data = i;
		}
	}

	logf_message(VERBOSE, "Memory type graphics: %u", memory_type_set.m_graphics_resources);
	logf_message(VERBOSE, "Memory type staging: %u", memory_type_set.m_resource_staging);
	logf_message(VERBOSE, "Memory type uniform: %u", memory_type_set.m_uniform_data);

	return memory_type_set;
}

void allocate_device_memory(VkDevice device, VkDeviceSize size, uint32_t memory_type, VkDeviceMemory *memory_ptr) {
	
	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = size;
	allocate_info.memoryTypeIndex = memory_type;

	vkAllocateMemory(device, &allocate_info, NULL, memory_ptr);
}

void bind_buffer_to_memory(buffer_t buffer, VkDeviceMemory memory, VkDeviceSize offset) {
	vkBindBufferMemory(buffer.m_device, buffer.m_handle, memory, offset);
}
