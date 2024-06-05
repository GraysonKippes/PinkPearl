#ifndef VK_TEXTURE_H
#define VK_TEXTURE_H

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "math/extent.h"
#include "util/string.h"

// Describes the usage of an image inside the GPU.
typedef struct TextureImageUsage {
	VkPipelineStageFlags2	pipelineStageMask;	// All pipeline stages in which the image is used.
	VkAccessFlags2			memoryAccessMask;	// All the ways in which the memory (i.e. contents) of the image is accessed.
	VkImageLayout			imageLayout;		// The current layout of the image.
} TextureImageUsage;

typedef struct TextureImage {
	VkImage vkImage;
	VkImageView vkImageView;
	TextureImageUsage usage;
} TextureImage;

typedef struct TextureImageSubresourceRange {
	VkImageAspectFlags imageAspectMask;
	uint32_t baseArrayLayer;
	uint32_t arrayLayerCount;
} TextureImageSubresourceRange;

typedef struct TextureAnimation {
	uint32_t startCell;
	uint32_t numFrames;
	uint32_t framesPerSecond;
} TextureAnimation;

typedef struct Texture {
	
	uint32_t numImageArrayLayers;
	TextureImage image;
	VkFormat format;
	
	uint32_t numAnimations;
	TextureAnimation *animations;
	
	VkDeviceMemory memory;
	VkDevice device;
	
} Texture;

typedef struct TextureCreateInfo {
	
	String textureID;
	
	bool isLoaded;
	bool isTilemap;

	// Number of cells in the texture, in each dimension.
	extent_t numCells;

	// The dimensions of each cell, in texels.
	extent_t cellExtent;

	uint32_t numAnimations;
	TextureAnimation *animations;

} TextureCreateInfo;

extern const TextureImageUsage imageUsageUndefined;
extern const TextureImageUsage imageUsageTransferSource;
extern const TextureImageUsage imageUsageTransferDestination;
extern const TextureImageUsage imageUsageComputeRead;
extern const TextureImageUsage imageUsageComputeWrite;
extern const TextureImageUsage imageUsageSampled;

// Returns a null texture struct.
Texture makeNullTexture(void);

// Returns true if the texture is "null" or empty, false otherwise.
bool textureIsNull(const Texture texture);

// Creates and returns a blank texture with undefined layout.
Texture createTexture(const TextureCreateInfo textureCreateInfo);

// Frees the memory (both CPU and GPU) used by the texture and resets the texture to null state.
// Returns true if executed successfully, false otherwise.
bool deleteTexture(Texture *const pTexture);

VkImageSubresourceRange makeImageSubresourceRange(const TextureImageSubresourceRange subresourceRange);

VkImageMemoryBarrier2 makeImageTransitionBarrier(const TextureImage image, const TextureImageSubresourceRange subresourceRange, const TextureImageUsage newUsage);

#endif	// VK_TEXTURE_H
