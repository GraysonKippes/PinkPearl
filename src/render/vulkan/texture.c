#include "texture.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log/logging.h"
#include "util/allocate.h"

#include "buffer.h"
#include "command_buffer.h"
#include "vulkan_manager.h"

const TextureImageUsage imageUsageUndefined = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
	.memoryAccessMask = VK_ACCESS_2_NONE,
	.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
};

const TextureImageUsage imageUsageTransferSource = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
	.memoryAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
};

const TextureImageUsage imageUsageTransferDestination = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
	.memoryAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
};

const TextureImageUsage imageUsageComputeRead = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
	.memoryAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_GENERAL
};

const TextureImageUsage imageUsageComputeWrite = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
	.memoryAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_GENERAL
};

const TextureImageUsage imageUsageSampled = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
	.memoryAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
};

Texture makeNullTexture(void) {
	return (Texture){
		.numAnimations = 0,
		.animations = NULL,
		.numImageArrayLayers = 0,
		.image = (TextureImage){ 
			.vkImage = VK_NULL_HANDLE,
			.vkImageView = VK_NULL_HANDLE,
			.usage = imageUsageUndefined
		},
		.format = VK_FORMAT_UNDEFINED,
		.memory = VK_NULL_HANDLE,
		.device = VK_NULL_HANDLE
	};
}

bool textureIsNull(const Texture texture) {
	return texture.numImageArrayLayers == 0
		|| texture.image.vkImage == VK_NULL_HANDLE
		|| texture.image.vkImageView == VK_NULL_HANDLE
		|| texture.memory == VK_NULL_HANDLE
		|| texture.device == VK_NULL_HANDLE;
}

static VkFormat getTextureImageFormat(const TextureCreateInfo textureCreateInfo) {
	if (textureCreateInfo.isTilemap) {
		return VK_FORMAT_R8G8B8A8_UINT;
	}
	return VK_FORMAT_R8G8B8A8_SRGB;
}

static VkImageUsageFlags getTextureImageUsage(const TextureCreateInfo textureCreateInfo) {
	if (textureCreateInfo.isTilemap) {
		return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	}
	return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
}

static VkImage createTextureImage(const Texture texture, const TextureCreateInfo textureCreateInfo) {

	uint32_t queue_family_indices[2];
	if (textureCreateInfo.isTilemap) {
		queue_family_indices[0] = *physical_device.queue_family_indices.transfer_family_ptr;
		queue_family_indices[1] = *physical_device.queue_family_indices.compute_family_ptr;
	}
	else {
		queue_family_indices[0] = *physical_device.queue_family_indices.graphics_family_ptr;
		queue_family_indices[1] = *physical_device.queue_family_indices.transfer_family_ptr;
	}

	const VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = texture.format,
		.extent.width = textureCreateInfo.cellExtent.width,
		.extent.height = textureCreateInfo.cellExtent.length,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = texture.numImageArrayLayers,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = getTextureImageUsage(textureCreateInfo),
		.sharingMode = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = 2,
		.pQueueFamilyIndices = queue_family_indices,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkImage vkImage = VK_NULL_HANDLE;
	const VkResult imageCreateResult = vkCreateImage(texture.device, &imageCreateInfo, NULL, &vkImage);
	if (imageCreateResult != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: image creation failed (error code: %i).", imageCreateResult);
	}
	return vkImage;
}

static void getImageMemoryRequirements(const VkDevice device, const VkImage vkImage, VkMemoryRequirements2 *const memory_requirements) {

	// Query memory requirements for this image.
	VkImageMemoryRequirementsInfo2 image_memory_requirements_info = { 0 };
	image_memory_requirements_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
	image_memory_requirements_info.pNext = NULL;
	image_memory_requirements_info.image = vkImage;

	VkMemoryRequirements2 image_memory_requirements = { 0 };
	image_memory_requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	image_memory_requirements.pNext = NULL;

	vkGetImageMemoryRequirements2(device, &image_memory_requirements_info, &image_memory_requirements);
	*memory_requirements = image_memory_requirements;
}

static void bindImageMemory(const Texture texture, const VkImage vkImage, VkDeviceSize offset) {

	// Bind the image to memory.
	const VkBindImageMemoryInfo image_bind_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
		.pNext = NULL,
		.image = vkImage,
		.memory = texture.memory,
		.memoryOffset = offset
	};

	vkBindImageMemory2(texture.device, 1, &image_bind_info);
}

static VkImageView createTextureImageView(const Texture texture, const VkImage vkImage) {

	// Subresource range used in all image views and layout transitions.
	static const VkImageSubresourceRange image_subresource_range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};

	// Create image view.
	const VkImageViewCreateInfo image_view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = vkImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		.format = texture.format,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange = image_subresource_range
	};

	VkImageView vkImageView = VK_NULL_HANDLE;
	const VkResult result = vkCreateImageView(texture.device, &image_view_create_info, NULL, &vkImageView);
	if (result != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: image view creation failed. (Error code: %i)", result);
	}
	return vkImageView;
}

Texture createTexture(const TextureCreateInfo textureCreateInfo) {

	Texture texture = makeNullTexture();
	texture.numAnimations = textureCreateInfo.numAnimations;
	texture.numImageArrayLayers = textureCreateInfo.numCells.width * textureCreateInfo.numCells.length;
	texture.format = getTextureImageFormat(textureCreateInfo);
	texture.device = device;	// TODO - pass vkDevice through parameters, not global state.

	allocate((void **)&texture.animations, texture.numAnimations, sizeof(TextureAnimation));
	memcpy_s(texture.animations, texture.numAnimations * sizeof(TextureAnimation), textureCreateInfo.animations, textureCreateInfo.numAnimations * sizeof(TextureAnimation));

	texture.image.vkImage = createTextureImage(texture, textureCreateInfo);

	// Allocate memory for the texture image.
	VkMemoryRequirements2 image_memory_requirements;
	getImageMemoryRequirements(texture.device, texture.image.vkImage, &image_memory_requirements);
	const VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = image_memory_requirements.memoryRequirements.size,
		.memoryTypeIndex = memory_type_set.graphics_resources
	};

	const VkResult memory_allocation_result = vkAllocateMemory(texture.device, &allocate_info, NULL, &texture.memory);
	if (memory_allocation_result != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: failed to allocate memory (error code: %i).", memory_allocation_result);
		deleteTexture(&texture);
		return texture;
	}

	bindImageMemory(texture, texture.image.vkImage, 0);
	texture.image.vkImageView = createTextureImageView(texture, texture.image.vkImage);

	return texture;
}

bool deleteTexture(Texture *const pTexture) {
	if (!pTexture) {
		return false;
	}
	
	deallocate((void **)&pTexture->animations);
	vkDestroyImage(pTexture->device, pTexture->image.vkImage, NULL);
	vkDestroyImageView(pTexture->device, pTexture->image.vkImageView, NULL);
	vkFreeMemory(pTexture->device, pTexture->memory, NULL);
	*pTexture = makeNullTexture();

	return true;
}

VkImageSubresourceRange makeImageSubresourceRange(const TextureImageSubresourceRange subresourceRange) {
	return (VkImageSubresourceRange){
		.aspectMask = subresourceRange.imageAspectMask,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = subresourceRange.baseArrayLayer,
		.layerCount = subresourceRange.layerCount
	};
}

VkImageMemoryBarrier2 makeImageTransitionBarrier(const TextureImage image, const TextureImageSubresourceRange subresourceRange, const TextureImageUsage newUsage) {
	return (VkImageMemoryBarrier2){
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = image.usage.pipelineStageMask,
		.srcAccessMask = image.usage.memoryAccessMask,
		.dstStageMask = newUsage.pipelineStageMask,
		.dstAccessMask = newUsage.memoryAccessMask,
		.oldLayout = image.usage.imageLayout,
		.newLayout = newUsage.imageLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image.vkImage,
		.subresourceRange = makeImageSubresourceRange(subresourceRange)
	};
}