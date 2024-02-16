#ifndef VK_TEXTURE_H
#define VK_TEXTURE_H

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "render/texture_info.h"

typedef struct texture_image_t {

	uint32_t image_array_length;
	VkImage vk_image;
	VkImageView vk_image_view;

	uint32_t frames_per_second;

} texture_image_t;

typedef struct texture_t {

	uint32_t num_images;
	texture_image_t *images;

	VkFormat format;
	VkImageLayout layout;
	VkDeviceMemory memory;

	// The device with which the images in this texture were created.
	VkDevice device;

} texture_t;

texture_t make_null_texture(void);

// Creates and returns a blank texture with undefined layout.
texture_t create_texture(texture_create_info_t texture_create_info);

bool destroy_texture(texture_t *restrict const texture_ptr);

#endif	// VK_TEXTURE_H
