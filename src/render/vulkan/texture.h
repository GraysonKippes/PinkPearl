#ifndef TEXTURE_H
#define TEXTURE_H

#include <vulkan/vulkan.h>

typedef struct texture_t {
	
	// Handles to corresponding Vulkan objects.
	VkImage m_vk_image = VK_NULL_HANDLE;
	VkImageView m_vk_image_view = VK_NULL_HANDLE;
	VkDeviceMemory m_vk_memory = VK_NULL_HANDLE;

} texture_t;

#endif	// TEXTURE_H
