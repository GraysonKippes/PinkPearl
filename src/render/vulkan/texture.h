#ifndef VK_TEXTURE_H
#define VK_TEXTURE_H

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "math/extent.h"
#include "util/string.h"

/* -- IMAGE OBJECT and FUNCTIONALITY -- */

// Describes the usage of an image inside the GPU.
typedef struct ImageUsage {
	VkPipelineStageFlags2	pipelineStageMask;	// All pipeline stages in which the image is used.
	VkAccessFlags2			memoryAccessMask;	// All the ways in which the memory (i.e. contents) of the image is accessed.
	VkImageLayout			imageLayout;		// The current layout of the image.
} ImageUsage;

typedef struct ImageSubresourceRange {
	VkImageAspectFlags 	imageAspectMask;
	uint32_t 			baseArrayLayer;
	uint32_t 			arrayLayerCount;
} ImageSubresourceRange;

typedef struct Image {

	// Description of image usage.
	ImageUsage usage;
	
	// Size of the texture in texels.
	Extent extent;
	
	// Handle to the Vulkan image.
	VkImage vkImage;

	// Handle to the Vulkan image view.
	VkImageView vkImageView;
	
	// Format of this image.
	VkFormat vkFormat;
	
	// The Vulkan device with which this image was created.
	VkDevice vkDevice;
	
} Image;

// Checks if the Vulkan object handles are not null and all the image properties are valid.
// Returns true if the image is valid, false otherwise.
bool validateImage(const Image image);

// Only checks if the Vulkan object handles are not null, meant to be used for object deletion.
// Returns true if the image is valid, false otherwise.
bool weaklyValidateImage(const Image image);

// Destroys the Vulkan objects associated with this image and resets all the handles and other image properties.
bool deleteImage(Image *const pImage);

/* -- TEXTURE OBJECT and FUNCTIONALITY -- */

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
	
	VkDeviceMemory vkDeviceMemory;
	
	VkDevice vkDevice;
	
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
	Extent numCells;

	// The dimensions of each cell, in texels.
	Extent cellExtent;

	uint32_t numAnimations;
	TextureAnimation *animations;

} TextureCreateInfo;

extern const ImageUsage imageUsageUndefined;
extern const ImageUsage imageUsageTransferSource;
extern const ImageUsage imageUsageTransferDestination;
extern const ImageUsage imageUsageComputeRead;
extern const ImageUsage imageUsageComputeWrite;
extern const ImageUsage imageUsageSampled;
extern const ImageUsage imageUsageDepthAttachment;

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

VkImageSubresourceLayers makeImageSubresourceLayers(const ImageSubresourceRange imageSubresourceRange);

VkImageSubresourceRange makeImageSubresourceRange(const ImageSubresourceRange imageSubresourceRange);

VkImageMemoryBarrier2 makeImageTransitionBarrier(const Image image, const ImageSubresourceRange subresourceRange, const ImageUsage newUsage);

#endif	// VK_TEXTURE_H
