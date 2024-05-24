#ifndef VK_TEXTURE_H
#define VK_TEXTURE_H

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "render/texture_info.h"

typedef struct TextureImage {
	VkImage vkImage;
	VkImageView vkImageView;
} TextureImage;

typedef struct TextureAnimation {
	uint32_t startCell;
	uint32_t numFrames;
	uint32_t framesPerSecond;
} TextureAnimation;

typedef struct Texture {
	
	uint32_t numAnimations;
	TextureAnimation *animations;
	
	uint32_t numImageArrayLayers;
	TextureImage image;
	VkFormat format;
	VkImageLayout layout;
	
	VkDeviceMemory memory;
	VkDevice device;
	
} Texture;

Texture make_null_texture(void);
bool is_texture_null(const Texture texture);

// Creates and returns a blank texture with undefined layout.
Texture create_texture(const texture_create_info_t texture_create_info);

bool destroy_texture(Texture *const texture_ptr);

#endif	// VK_TEXTURE_H
