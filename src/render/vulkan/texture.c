#include "texture.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "log/logging.h"
#include "util/allocate.h"

#include "buffer.h"
#include "command_buffer.h"
#include "vulkan_manager.h"

texture_t make_null_texture(void) {
	return (texture_t){
		.num_animations = 0,
		.animations = NULL,
		.num_image_array_layers = 0,
		.image = (texture_image_t){ 0 },
		.format = VK_FORMAT_UNDEFINED,
		.layout = VK_IMAGE_LAYOUT_UNDEFINED,
		.memory = VK_NULL_HANDLE,
		.device = VK_NULL_HANDLE
	};
}

bool is_texture_null(const texture_t texture) {
	return texture.num_image_array_layers == 0
		|| texture.image.vk_image == VK_NULL_HANDLE
		|| texture.image.vk_image_view == VK_NULL_HANDLE
		|| texture.memory == VK_NULL_HANDLE
		|| texture.device == VK_NULL_HANDLE;
}

static VkFormat texture_image_format(const texture_type_t texture_type) {
	switch (texture_type) {
		case TEXTURE_TYPE_NORMAL: return VK_FORMAT_R8G8B8A8_SRGB;
		case TEXTURE_TYPE_TILEMAP: return VK_FORMAT_R8G8B8A8_UINT;
	}
	return VK_FORMAT_UNDEFINED;
}

static VkImageUsageFlags texture_image_usage(const texture_type_t texture_type) {
	switch (texture_type) {
		case TEXTURE_TYPE_NORMAL: return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		case TEXTURE_TYPE_TILEMAP: return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	}
	return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
}

static VkImage create_texture_image(const texture_t texture, const texture_create_info_t texture_create_info) {

	uint32_t queue_family_indices[2];
	if (texture_create_info.type == TEXTURE_TYPE_TILEMAP) {
		queue_family_indices[0] = *physical_device.queue_family_indices.transfer_family_ptr;
		queue_family_indices[1] = *physical_device.queue_family_indices.compute_family_ptr;
	}
	else {
		queue_family_indices[0] = *physical_device.queue_family_indices.graphics_family_ptr;
		queue_family_indices[1] = *physical_device.queue_family_indices.transfer_family_ptr;
	}

	const VkImageCreateInfo image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = texture.format,
		.extent.width = texture_create_info.cell_extent.width,
		.extent.height = texture_create_info.cell_extent.length,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = texture_create_info.num_cells.width * texture_create_info.num_cells.length,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = texture_image_usage(texture_create_info.type),
		.sharingMode = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = 2,
		.pQueueFamilyIndices = queue_family_indices,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkImage vk_image = VK_NULL_HANDLE;
	const VkResult image_create_result = vkCreateImage(texture.device, &image_create_info, NULL, &vk_image);
	if (image_create_result != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: image creation failed (error code: %i).", image_create_result);
	}
	return vk_image;
}

static void get_image_memory_requirements(const VkDevice device, const VkImage vk_image, VkMemoryRequirements2 *const memory_requirements) {

	// Query memory requirements for this image.
	VkImageMemoryRequirementsInfo2 image_memory_requirements_info = { 0 };
	image_memory_requirements_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
	image_memory_requirements_info.pNext = NULL;
	image_memory_requirements_info.image = vk_image;

	VkMemoryRequirements2 image_memory_requirements = { 0 };
	image_memory_requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	image_memory_requirements.pNext = NULL;

	vkGetImageMemoryRequirements2(device, &image_memory_requirements_info, &image_memory_requirements);
	*memory_requirements = image_memory_requirements;
}

static void bind_image_memory(const texture_t texture, const VkImage vk_image, VkDeviceSize offset) {

	// Bind the image to memory.
	const VkBindImageMemoryInfo image_bind_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
		.pNext = NULL,
		.image = vk_image,
		.memory = texture.memory,
		.memoryOffset = offset
	};

	vkBindImageMemory2(texture.device, 1, &image_bind_info);
}

static VkImageView create_texture_image_view(const texture_t texture, const VkImage vk_image) {

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
		.image = vk_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		.format = texture.format,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange = image_subresource_range
	};

	VkImageView vk_image_view = VK_NULL_HANDLE;
	const VkResult result = vkCreateImageView(texture.device, &image_view_create_info, NULL, &vk_image_view);
	if (result != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: image view creation failed. (Error code: %i)", result);
	}
	return vk_image_view;
}

texture_t create_texture(const texture_create_info_t texture_create_info) {

	texture_t texture = make_null_texture();
	texture.num_animations = texture_create_info.num_animations;
	texture.num_image_array_layers = texture_create_info.num_cells.width * texture_create_info.num_cells.length;
	texture.format = texture_image_format(texture_create_info.type);
	texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	texture.memory = VK_NULL_HANDLE;
	texture.device = device;	// TODO - pass vkDevice through argument list, not global state.

	allocate((void **)&texture.animations, texture.num_animations, sizeof(texture_animation_t));
	for (uint32_t i = 0; i < texture.num_animations; ++i) {
		texture.animations[i].start_cell = texture_create_info.animations[i].start_cell;
		texture.animations[i].num_frames = texture_create_info.animations[i].num_frames;
		texture.animations[i].frames_per_second = texture_create_info.animations[i].frames_per_second;
	}

	texture.image.vk_image = create_texture_image(texture, texture_create_info);

	// Allocate memory for the texture image.
	VkMemoryRequirements2 image_memory_requirements;
	get_image_memory_requirements(texture.device, texture.image.vk_image, &image_memory_requirements);
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

	bind_image_memory(texture, texture.image.vk_image, 0);
	texture.image.vk_image_view = create_texture_image_view(texture, texture.image.vk_image);

	return texture;
}

bool destroy_texture(texture_t *const texture_ptr) {
	if (texture_ptr == NULL) {
		return false;
	}
	
	deallocate((void **)&texture_ptr->animations);
	vkDestroyImage(texture_ptr->device, texture_ptr->image.vk_image, NULL);
	vkDestroyImageView(texture_ptr->device, texture_ptr->image.vk_image_view, NULL);
	vkFreeMemory(texture_ptr->device, texture_ptr->memory, NULL);
	*texture_ptr = make_null_texture();

	return true;
}
