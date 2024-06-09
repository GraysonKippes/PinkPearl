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

const ImageUsage imageUsageUndefined = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
	.memoryAccessMask = VK_ACCESS_2_NONE,
	.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
};

const ImageUsage imageUsageTransferSource = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
	.memoryAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
};

const ImageUsage imageUsageTransferDestination = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
	.memoryAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
};

const ImageUsage imageUsageComputeRead = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
	.memoryAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_GENERAL
};

const ImageUsage imageUsageComputeWrite = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
	.memoryAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_GENERAL
};

const ImageUsage imageUsageSampled = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
	.memoryAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
};

Texture makeNullTexture(void) {
	return (Texture){
		.numAnimations = 0,
		.animations = nullptr,
		.numImageArrayLayers = 0,
		.image = (Image){ 
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
		.pNext = nullptr,
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
	const VkResult imageCreateResult = vkCreateImage(texture.device, &imageCreateInfo, nullptr, &vkImage);
	if (imageCreateResult != VK_SUCCESS) {
		logMsgF(ERROR, "Error loading texture: image creation failed (error code: %i).", imageCreateResult);
	}
	return vkImage;
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
		.pNext = nullptr,
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
	const VkResult result = vkCreateImageView(texture.device, &image_view_create_info, nullptr, &vkImageView);
	if (result != VK_SUCCESS) {
		logMsgF(ERROR, "Error loading texture: image view creation failed. (Error code: %i)", result);
	}
	return vkImageView;
}

Texture createTexture(const TextureCreateInfo textureCreateInfo) {

	Texture texture = makeNullTexture();
	texture.numAnimations = textureCreateInfo.numAnimations;
	texture.numImageArrayLayers = textureCreateInfo.numCells.width * textureCreateInfo.numCells.length;
	texture.format = getTextureImageFormat(textureCreateInfo);
	texture.device = device;	// TODO - pass vkDevice through parameters, not global state.
	texture.isLoaded = textureCreateInfo.isLoaded;
	texture.isTilemap = textureCreateInfo.isTilemap;

	allocate((void **)&texture.animations, texture.numAnimations, sizeof(TextureAnimation));
	memcpy_s(texture.animations, texture.numAnimations * sizeof(TextureAnimation), textureCreateInfo.animations, textureCreateInfo.numAnimations * sizeof(TextureAnimation));

	texture.image.vkImage = createTextureImage(texture, textureCreateInfo);
	texture.image.extent = textureCreateInfo.cellExtent;

	// Allocate memory for the texture image.
	const VkMemoryRequirements2 imageMemoryRequirements = getImageMemoryRequirements(texture.device, texture.image.vkImage);
	const VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = imageMemoryRequirements.memoryRequirements.size,
		.memoryTypeIndex = memory_type_set.graphics_resources
	};

	const VkResult memoryAllocationResult = vkAllocateMemory(texture.device, &allocateInfo, nullptr, &texture.memory);
	if (memoryAllocationResult != VK_SUCCESS) {
		logMsgF(ERROR, "Error loading texture: failed to allocate memory (error code: %i).", memoryAllocationResult);
		deleteTexture(&texture);
		return texture;
	}

	bindImageMemory(texture.device, texture.image.vkImage, texture.memory, 0);
	texture.image.vkImageView = createTextureImageView(texture, texture.image.vkImage);

	/* Transition texture image layout to something usable. */

	VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
	allocate_command_buffers(texture.device, cmdPoolGraphics, 1, &cmdBuf);
	cmdBufBegin(cmdBuf, true); {
		
		ImageUsage imageUsage;
		if (textureCreateInfo.isLoaded) {
			imageUsage = imageUsageTransferDestination;
		} else {
			imageUsage = imageUsageSampled;
		}
		
		const ImageSubresourceRange imageSubresourceRange = {
			.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseArrayLayer = 0,
			.arrayLayerCount = VK_REMAINING_ARRAY_LAYERS
		};
		
		const VkImageMemoryBarrier2 imageMemoryBarrier = makeImageTransitionBarrier(texture.image, imageSubresourceRange, imageUsage);
		const VkDependencyInfo dependencyInfo = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.dependencyFlags = 0,
			.memoryBarrierCount = 0,
			.pMemoryBarriers = nullptr,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers = nullptr,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &imageMemoryBarrier
		};
		vkCmdPipelineBarrier2(cmdBuf, &dependencyInfo);
		texture.image.usage = imageUsage;
		
	} vkEndCommandBuffer(cmdBuf);
	submit_command_buffers_async(queueGraphics, 1, &cmdBuf);

	return texture;
}

bool deleteTexture(Texture *const pTexture) {
	if (!pTexture) {
		return false;
	}
	
	deallocate((void **)&pTexture->animations);
	vkDestroyImage(pTexture->device, pTexture->image.vkImage, nullptr);
	vkDestroyImageView(pTexture->device, pTexture->image.vkImageView, nullptr);
	vkFreeMemory(pTexture->device, pTexture->memory, nullptr);
	*pTexture = makeNullTexture();

	return true;
}

VkMemoryRequirements2 getImageMemoryRequirements(const VkDevice vkDevice, const VkImage vkImage) {

	// Query memory requirements for this image.
	const VkImageMemoryRequirementsInfo2 imageMemoryRequirementsInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
		.pNext = nullptr,
		.image = vkImage
	};

	VkMemoryRequirements2 imageMemoryRequirements = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
		.pNext = nullptr
	};

	vkGetImageMemoryRequirements2(vkDevice, &imageMemoryRequirementsInfo, &imageMemoryRequirements);
	return imageMemoryRequirements;
}

void bindImageMemory(const VkDevice vkDevice, const VkImage vkImage, const VkDeviceMemory vkDeviceMemory, const VkDeviceSize offset) {
	const VkBindImageMemoryInfo bindInfo = {
		.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
		.pNext = nullptr,
		.image = vkImage,
		.memory = vkDeviceMemory,
		.memoryOffset = offset
	};
	vkBindImageMemory2(vkDevice, 1, &bindInfo);
}

VkImageSubresourceLayers makeImageSubresourceLayers(const ImageSubresourceRange subresourceRange) {
	return (VkImageSubresourceLayers){
		.aspectMask = subresourceRange.imageAspectMask,
		.mipLevel = 0,
		.baseArrayLayer = subresourceRange.baseArrayLayer,
		.layerCount = subresourceRange.arrayLayerCount
	};
}

VkImageSubresourceRange makeImageSubresourceRange(const ImageSubresourceRange subresourceRange) {
	return (VkImageSubresourceRange){
		.aspectMask = subresourceRange.imageAspectMask,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = subresourceRange.baseArrayLayer,
		.layerCount = subresourceRange.arrayLayerCount
	};
}

VkImageMemoryBarrier2 makeImageTransitionBarrier(const Image image, const ImageSubresourceRange subresourceRange, const ImageUsage newUsage) {
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