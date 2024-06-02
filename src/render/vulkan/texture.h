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

Texture makeNullTexture(void);
bool textureIsNull(const Texture texture);

// Creates and returns a blank texture with undefined layout.
Texture createTexture(const TextureCreateInfo texture_create_info);

bool destroy_texture(Texture *const texture_ptr);

#endif	// VK_TEXTURE_H
