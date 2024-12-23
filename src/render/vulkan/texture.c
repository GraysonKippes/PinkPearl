#include "texture.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log/Logger.h"
#include "util/allocate.h"

#include "buffer.h"
#include "CommandBuffer.h"
#include "VulkanManager.h"

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

const ImageUsage imageUsageColorAttachment = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
	.memoryAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
};

const ImageUsage imageUsageDepthAttachment = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
	.memoryAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
	.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
};

const ImageUsage imageUsagePresent = {
	.pipelineStageMask = VK_PIPELINE_STAGE_2_NONE,
	.memoryAccessMask = VK_ACCESS_2_NONE,
	.imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
};

bool validateImage(const Image image) {
	return image.usage.pipelineStageMask != VK_PIPELINE_STAGE_2_NONE
		&& image.usage.memoryAccessMask != VK_ACCESS_NONE
		&& image.usage.imageLayout != VK_IMAGE_LAYOUT_UNDEFINED
		&& image.extent.length > 0
		&& image.extent.width > 0
		&& image.vkImage != VK_NULL_HANDLE
		&& image.vkImageView != VK_NULL_HANDLE
		&& image.vkFormat != VK_FORMAT_UNDEFINED
		&& image.vkDevice != VK_NULL_HANDLE;
}

bool weaklyValidateImage(const Image image) {
	return image.vkImage != VK_NULL_HANDLE
		&& image.vkImageView != VK_NULL_HANDLE
		&& image.vkDevice != VK_NULL_HANDLE;
}

bool deleteImage(Image *const pImage) {
	if (!pImage) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting image: pointer to image object is null.");
		return false;
	} else if (!weaklyValidateImage(*pImage)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting image: image object is invalid (image = { .usage = { .pipelineStageMask = 0x%llX, .memoryAccessMask = 0x%llX, .imageLayout = 0x%llX }, .extent = { .width = %u, .length = %u }, .vkImage = %p, .vkImageView = %p, vkFormat = 0x%llX, .vkDevice = %p }).", 
				pImage->usage.pipelineStageMask, pImage->usage.memoryAccessMask, pImage->usage.imageLayout, pImage->extent.width, pImage->extent.length, pImage->vkImage, pImage->vkImageView, pImage->vkFormat, pImage->vkDevice);
		return false;
	}
	
	// Destroy the Vulkan objects associated with the image struct.
	vkDestroyImage(pImage->vkDevice, pImage->vkImage, nullptr);
	vkDestroyImageView(pImage->vkDevice, pImage->vkImageView, nullptr);
	
	// Nullify the handles inside the struct.
	pImage->vkImage = VK_NULL_HANDLE;
	pImage->vkImageView = VK_NULL_HANDLE;
	pImage->vkDevice = VK_NULL_HANDLE;
	
	// Reset other information attached to the image.
	pImage->arrayLayerCount = 0;
	pImage->usage = imageUsageUndefined;
	pImage->extent = (Extent){ 0, 0 };

	return true;
}

const Texture nullTexture = {
	.isLoaded = false,
	.isTilemap = false,
	.numAnimations = 0,
	.animations = nullptr,
	.image.usage = imageUsageUndefined,
	.image.extent.width = 0,
	.image.extent.length = 0,
	.image.vkImage = VK_NULL_HANDLE,
	.image.vkImageView = VK_NULL_HANDLE,
	.image.vkFormat = VK_FORMAT_UNDEFINED,
	.image.vkDevice = VK_NULL_HANDLE,
	.vkDeviceMemory = VK_NULL_HANDLE,
	.vkDevice = VK_NULL_HANDLE
};

bool textureIsNull(const Texture texture) {
	return texture.image.vkImage == VK_NULL_HANDLE
		|| texture.image.vkImageView == VK_NULL_HANDLE
		|| texture.vkDeviceMemory == VK_NULL_HANDLE
		|| texture.vkDevice == VK_NULL_HANDLE;
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

static VkImage createTextureImage(const VkDevice vkDevice, const uint32_t arrayLayerCount, const TextureCreateInfo textureCreateInfo) {

	uint32_t queueFamilyIndices[2];
	if (textureCreateInfo.isTilemap) {
		queueFamilyIndices[0] = *physical_device.queueFamilyIndices.transfer_family_ptr;
		queueFamilyIndices[1] = *physical_device.queueFamilyIndices.compute_family_ptr;
	} else {
		queueFamilyIndices[0] = *physical_device.queueFamilyIndices.graphics_family_ptr;
		queueFamilyIndices[1] = *physical_device.queueFamilyIndices.transfer_family_ptr;
	}

	const VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = getTextureImageFormat(textureCreateInfo),
		.extent.width = textureCreateInfo.cellExtent.width,
		.extent.height = textureCreateInfo.cellExtent.length,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = arrayLayerCount,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = getTextureImageUsage(textureCreateInfo),
		.sharingMode = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = 2,
		.pQueueFamilyIndices = queueFamilyIndices,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkImage vkImage = VK_NULL_HANDLE;
	const VkResult imageCreateResult = vkCreateImage(vkDevice, &imageCreateInfo, nullptr, &vkImage);
	if (imageCreateResult != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating texture: image creation failed (error code: %i).", imageCreateResult);
	}
	
	return vkImage;
}

static VkImageView createTextureImageView(const Texture texture, const TextureCreateInfo textureCreateInfo, const VkImage vkImage) {

	// Subresource range used in all image views and layout transitions.
	static const VkImageSubresourceRange imageSubresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};

	// Create image view.
	const VkImageViewCreateInfo imageViewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = vkImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		.format = getTextureImageFormat(textureCreateInfo),
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange = imageSubresourceRange
	};

	VkImageView vkImageView = VK_NULL_HANDLE;
	const VkResult result = vkCreateImageView(texture.vkDevice, &imageViewCreateInfo, nullptr, &vkImageView);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating texture: image view creation failed (error code: %i).", result);
	}
	return vkImageView;
}

Texture createTexture(const TextureCreateInfo textureCreateInfo) {

	Texture texture = nullTexture;
	texture.numAnimations = textureCreateInfo.numAnimations;
	texture.vkDevice = device;	// TODO - pass vkDevice through parameters, not global state.
	texture.isLoaded = textureCreateInfo.isLoaded;
	texture.isTilemap = textureCreateInfo.isTilemap;

	allocate((void **)&texture.animations, texture.numAnimations, sizeof(TextureAnimation));
	memcpy(texture.animations, textureCreateInfo.animations, textureCreateInfo.numAnimations * sizeof(TextureAnimation));

	texture.image.arrayLayerCount = textureCreateInfo.numCells.width * textureCreateInfo.numCells.length;
	texture.image.vkImage = createTextureImage(texture.vkDevice, texture.image.arrayLayerCount, textureCreateInfo);
	texture.image.extent = textureCreateInfo.cellExtent;

	// Allocate memory for the texture image.
	const VkMemoryRequirements2 imageMemoryRequirements = getImageMemoryRequirements(texture.vkDevice, texture.image.vkImage);
	const VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = imageMemoryRequirements.memoryRequirements.size,
		.memoryTypeIndex = memory_type_set.graphics_resources
	};

	const VkResult memoryAllocationResult = vkAllocateMemory(texture.vkDevice, &allocateInfo, nullptr, &texture.vkDeviceMemory);
	if (memoryAllocationResult != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error loading texture: failed to allocate memory (error code: %i).", memoryAllocationResult);
		deleteTexture(&texture);
		return texture;
	}

	bindImageMemory(texture.vkDevice, texture.image.vkImage, texture.vkDeviceMemory, 0);
	texture.image.vkImageView = createTextureImageView(texture, textureCreateInfo, texture.image.vkImage);
	texture.image.vkDevice = texture.vkDevice;

	/* Transition texture image layout to something usable. */

	CmdBufArray cmdBufArray = cmdBufAlloc(commandPoolGraphics, 1);
	cmdBufBegin(cmdBufArray, 0, true); {
		const ImageUsage imageUsage = textureCreateInfo.isLoaded ? imageUsageTransferDestination : imageUsageSampled;
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
		vkCmdPipelineBarrier2(cmdBufArray.pCmdBufs[0], &dependencyInfo);
		texture.image.usage = imageUsage;
	} cmdBufEnd(cmdBufArray, 0);
	submit_command_buffers_async(queueGraphics, 1, &cmdBufArray.pCmdBufs[0]);
	cmdBufFree(&cmdBufArray);

	return texture;
}

bool deleteTexture(Texture *const pTexture) {
	if (!pTexture) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting texture: pointer to texture object is null.");
		return false;
	} else if (textureIsNull(*pTexture)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting texture: texture object is invalid.");
		return false;
	}
	
	// TODO - implement allocator objects for each type of allocation (i.e. heap, stack, text, arena).
	//deallocate((void **)&pTexture->animations);
	vkDestroyImage(pTexture->vkDevice, pTexture->image.vkImage, nullptr);
	vkDestroyImageView(pTexture->vkDevice, pTexture->image.vkImageView, nullptr);
	vkFreeMemory(pTexture->vkDevice, pTexture->vkDeviceMemory, nullptr);
	*pTexture = nullTexture;

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

Sampler createSampler(VkDevice vkDevice, PhysicalDevice physicalDevice) {
	
	static const VkFilter filter = VK_FILTER_NEAREST;
	static const VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	
	const VkSamplerCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = filter,
		.minFilter = filter,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = addressMode,
		.addressModeV = addressMode,
		.addressModeW = addressMode,
		.mipLodBias = 0.0F,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = physicalDevice.properties.limits.maxSamplerAnisotropy,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0F,
		.maxLod = 0.0F,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};
	
	VkSampler vkSampler = VK_NULL_HANDLE;
	const VkResult result = vkCreateSampler(vkDevice, &createInfo, nullptr, &vkSampler);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating sampler: failed to create sampler (error code: %i).", result);
		return (Sampler){ .vkSampler = VK_NULL_HANDLE, .vkDevice = VK_NULL_HANDLE };
	}
	
	return (Sampler){ .vkSampler = vkSampler, .vkDevice = vkDevice };
}

void deleteSampler(Sampler *const pSampler) {
	if (!pSampler) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting sampler: pointer to sampler object is null.");
	}
	
	vkDestroySampler(pSampler->vkDevice, pSampler->vkSampler, nullptr);
	pSampler->vkSampler = VK_NULL_HANDLE;
	pSampler->vkDevice = VK_NULL_HANDLE;
}