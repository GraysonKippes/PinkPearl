#ifndef VK_TEXTURE_H
#define VK_TEXTURE_H

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "math/extent.h"
#include "util/string.h"

// Describes the usage of an image inside the GPU.
typedef struct ImageUsage {
	VkPipelineStageFlags2	pipelineStageMask;	// All pipeline stages in which the image is used.
	VkAccessFlags2			memoryAccessMask;	// All the ways in which the memory (i.e. contents) of the image is accessed.
	VkImageLayout			imageLayout;		// The current layout of the image.
} ImageUsage;

typedef struct Image {
	VkImage 	vkImage;		// Handle to the Vulkan image.
	VkImageView vkImageView;	// Handle to the Vulkan image view.
	VkFormat	vkFormat;		// Format of this image.
	ImageUsage 	usage;			// Description of image usage.
} Image;

typedef struct ImageSubresourceRange {
	VkImageAspectFlags 	imageAspectMask;
	uint32_t 			baseArrayLayer;
	uint32_t 			arrayLayerCount;
} ImageSubresourceRange;

typedef struct TextureAnimation {
	uint32_t startCell;
	uint32_t numFrames;
	uint32_t framesPerSecond;
} TextureAnimation;

typedef struct Texture {
	
	uint32_t numImageArrayLayers;
	Image image;
	[[deprecated("moving format to iamge struct.")]]
	VkFormat format;
	
	VkDeviceMemory memory;
	VkDevice device;
	
	bool isLoaded;
	bool isTilemap;
	
	uint32_t numAnimations;
	TextureAnimation *animations;
	
} Texture;

typedef struct TextureCreateInfo {
	
	String textureID;
	
	bool isLoaded;	// True if an image is loaded into the texture's image.
	bool isTilemap;	// True if the texture is to be used for stitching other textures.

	// Number of cells in the texture, in each dimension.
	extent_t numCells;

	// The dimensions of each cell, in texels.
	extent_t cellExtent;

	uint32_t numAnimations;
	TextureAnimation *animations;

} TextureCreateInfo;

extern const ImageUsage imageUsageUndefined;
extern const ImageUsage imageUsageTransferSource;
extern const ImageUsage imageUsageTransferDestination;
extern const ImageUsage imageUsageComputeRead;
extern const ImageUsage imageUsageComputeWrite;
extern const ImageUsage imageUsageSampled;

// Returns a null texture struct.
Texture makeNullTexture(void);

// Returns true if the texture is "null" or empty, false otherwise.
bool textureIsNull(const Texture texture);

// Creates and returns a blank texture with undefined layout.
Texture createTexture(const TextureCreateInfo textureCreateInfo);

// Frees the memory (both CPU and GPU) used by the texture and resets the texture to null state.
// Returns true if executed successfully, false otherwise.
bool deleteTexture(Texture *const pTexture);

VkMemoryRequirements2 getImageMemoryRequirements(const VkDevice vkDevice, const VkImage vkImage);

void bindImageMemory(const VkDevice vkDevice, const VkImage vkImage, const VkDeviceMemory vkDeviceMemory, const VkDeviceSize offset);

VkImageSubresourceRange makeImageSubresourceRange(const ImageSubresourceRange subresourceRange);

VkImageMemoryBarrier2 makeImageTransitionBarrier(const Image image, const ImageSubresourceRange subresourceRange, const ImageUsage newUsage);

#endif	// VK_TEXTURE_H
