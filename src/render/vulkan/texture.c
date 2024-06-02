#include "texture.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "log/logging.h"
#include "util/allocate.h"

#include "buffer.h"
#include "command_buffer.h"
#include "vulkan_manager.h"

Texture makeNullTexture(void) {
	return (Texture){
		.numAnimations = 0,
		.animations = NULL,
		.numImageArrayLayers = 0,
		.image = (TextureImage){ 0 },
		.format = VK_FORMAT_UNDEFINED,
		.layout = VK_IMAGE_LAYOUT_UNDEFINED,
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

static VkFormat texture_image_format(const TextureCreateInfo textureCreateInfo) {
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

static VkImage create_texture_image(const Texture texture, const TextureCreateInfo textureCreateInfo) {

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
		.extent.width = textureCreateInfo.cell_extent.width,
		.extent.height = textureCreateInfo.cell_extent.length,
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

static void get_image_memory_requirements(const VkDevice device, const VkImage vkImage, VkMemoryRequirements2 *const memory_requirements) {

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

static void bind_image_memory(const Texture texture, const VkImage vkImage, VkDeviceSize offset) {

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

static VkImageView create_texture_image_view(const Texture texture, const VkImage vkImage) {

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
	
	texture.numAnimations = textureCreateInfo.num_animations;
	texture.numImageArrayLayers = textureCreateInfo.num_cells.width * textureCreateInfo.num_cells.length;
	texture.format = texture_image_format(textureCreateInfo);
	texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	texture.memory = VK_NULL_HANDLE;
	texture.device = device;	// TODO - pass vkDevice through argument list, not global state.

	allocate((void **)&texture.animations, texture.numAnimations, sizeof(TextureAnimation));
	for (uint32_t i = 0; i < texture.numAnimations; ++i) {
		texture.animations[i].startCell = textureCreateInfo.animations[i].start_cell;
		texture.animations[i].numFrames = textureCreateInfo.animations[i].num_frames;
		texture.animations[i].framesPerSecond = textureCreateInfo.animations[i].frames_per_second;
	}

	texture.image.vkImage = create_texture_image(texture, textureCreateInfo);

	// Allocate memory for the texture image.
	VkMemoryRequirements2 image_memory_requirements;
	get_image_memory_requirements(texture.device, texture.image.vkImage, &image_memory_requirements);
	const VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = image_memory_requirements.memoryRequirements.size,
		.memoryTypeIndex = memory_type_set.graphics_resources
	};

	const VkResult memory_allocation_result = vkAllocateMemory(texture.device, &allocate_info, NULL, &texture.memory);
	if (memory_allocation_result != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: failed to allocate memory (error code: %i).", memory_allocation_result);
		destroy_texture(&texture);
		return texture;
	}

	bind_image_memory(texture, texture.image.vkImage, 0);
	texture.image.vkImageView = create_texture_image_view(texture, texture.image.vkImage);

	return texture;
}

bool destroy_texture(Texture *const pTexture) {
	if (pTexture == NULL) {
		return false;
	}
	
	deallocate((void **)&pTexture->animations);
	vkDestroyImage(pTexture->device, pTexture->image.vkImage, NULL);
	vkDestroyImageView(pTexture->device, pTexture->image.vkImageView, NULL);
	vkFreeMemory(pTexture->device, pTexture->memory, NULL);
	*pTexture = makeNullTexture();

	return true;
}
