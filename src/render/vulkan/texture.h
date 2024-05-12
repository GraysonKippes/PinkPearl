#ifndef VK_TEXTURE_H
#define VK_TEXTURE_H

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "render/texture_info.h"

typedef struct texture_image_t {
	VkImage vk_image;
	VkImageView vk_image_view;
} texture_image_t;

typedef struct texture_animation_t {
	uint32_t start_cell;
	uint32_t num_frames;
	uint32_t frames_per_second;
} texture_animation_t;

typedef struct texture_t {
	
	uint32_t num_animations;
	texture_animation_t *animations;
	
	uint32_t num_image_array_layers;
	texture_image_t image;
	VkFormat format;
	VkImageLayout layout;
	
	VkDeviceMemory memory;
	VkDevice device;
	
} texture_t;

texture_t make_null_texture(void);
bool is_texture_null(const texture_t texture);

// Creates and returns a blank texture with undefined layout.
texture_t create_texture(const texture_create_info_t texture_create_info);

bool destroy_texture(texture_t *const texture_ptr);

#endif	// VK_TEXTURE_H
