#include "memory.h"

#include <stddef.h>

#include "log/Logger.h"
#include "util/allocate.h"

memory_type_set_t select_memory_types(VkPhysicalDevice physical_device) {

	// TODO - redesign this whole function to be flexible with other types of GPUs.
	// 	Currently it only really targets NVIDIA/AMD GPUs.

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Selecting GPU memory types...");

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
			memory_type_set.graphics_resources = i;
		}

		// Resource staging memory type:
		// This memory type is to be used for buffers that stage data to the GPU.
		if (!(property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) 
				&& (property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
			memory_type_set.resource_staging = i;
		}

		// Uniform buffer memory type:
		// This memory type is to be used for small reused uniform buffers that updated very often (i.e. on a per-frame basis).
		if ((property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) 
				&& (property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
			memory_type_set.uniform_data = i;
		}
	}

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Memory type graphics: %u", memory_type_set.graphics_resources);
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Memory type staging: %u", memory_type_set.resource_staging);
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Memory type uniform: %u", memory_type_set.uniform_data);

	return memory_type_set;
}

VkDeviceSize align_offset(const VkDeviceSize offset, const VkDeviceSize alignment) {
	const VkDeviceSize remainder = offset % alignment;
	if (remainder != 0) {
		return offset + (alignment - remainder);
	}
	return offset;
}
