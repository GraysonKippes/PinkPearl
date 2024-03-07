#include "image.h"

#include <stdint.h>

#include "command_buffer.h"

static const VkImageSubresourceRange image_subresource_range_default = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseMipLevel = 0,
	.levelCount = 1,
	.baseArrayLayer = 0,
	.layerCount = VK_REMAINING_ARRAY_LAYERS
};

const VkImageSubresourceLayers image_subresource_layers_default = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.mipLevel = 0,
	.baseArrayLayer = 0,
	.layerCount = VK_REMAINING_ARRAY_LAYERS
};

image_t create_image(const image_create_info_t image_create_info) {
	
	image_t image = { 0 };

	image.num_array_layers = image_create_info.num_image_array_layers;
	image.format = image_create_info.format;
	image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	image.device = image_create_info.device;

	VkImageCreateInfo vk_image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = image.format,
		.extent.width = image_create_info.image_dimensions.width,
		.extent.height = image_create_info.image_dimensions.length,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = image.num_array_layers,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = image_create_info.usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.initialLayout = image.layout
	};

	if (!is_queue_family_set_null(image_create_info.queue_family_set)) {
		vk_image_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		vk_image_create_info.queueFamilyIndexCount = image_create_info.queue_family_set.num_queue_families;
		vk_image_create_info.pQueueFamilyIndices = image_create_info.queue_family_set.queue_families;
	}
	
	vkCreateImage(image.device, &vk_image_create_info, NULL, &image.vk_image);

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(image.device, image.vk_image, &memory_requirements);

	const VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = image_create_info.memory_type_index
	};

	vkAllocateMemory(image.device, &allocate_info, NULL, &image.memory);

	const VkBindImageMemoryInfo image_bind_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
		.pNext = NULL,
		.image = image.vk_image,
		.memory = image.memory,
		.memoryOffset = 0
	};

	vkBindImageMemory2(image.device, 1, &image_bind_info);

	const VkImageViewCreateInfo image_view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = image.vk_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		.format = image.format,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange = image_subresource_range_default
	};

	vkCreateImageView(image.device, &image_view_create_info, NULL, &image.vk_image_view);

	return image;
}

bool destroy_image(image_t *const image_ptr) {
	
	if (image_ptr == NULL) {
		return false;
	}
	
	vkDestroyImage(image_ptr->device, image_ptr->vk_image, NULL);
	image_ptr->vk_image = VK_NULL_HANDLE;
	
	vkDestroyImageView(image_ptr->device, image_ptr->vk_image_view, NULL);
	image_ptr->vk_image_view = VK_NULL_HANDLE;
	
	vkFreeMemory(image_ptr->device, image_ptr->memory, NULL);
	image_ptr->memory = VK_NULL_HANDLE;
	
	image_ptr->num_array_layers = 0;
	image_ptr->format = VK_FORMAT_UNDEFINED;
	image_ptr->layout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_ptr->device = VK_NULL_HANDLE;
	
	return true;
}

void transition_image_layout(VkQueue queue, VkCommandPool command_pool, image_t *image_ptr, image_layout_transition_t image_layout_transition) {

	if (image_ptr == NULL) {
		return;
	}

	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(image_ptr->device, command_pool, 1, &command_buffer);
	begin_command_buffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageMemoryBarrier memory_barrier = { 0 };
	memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memory_barrier.pNext = NULL;
	memory_barrier.srcAccessMask = VK_ACCESS_NONE;
	memory_barrier.dstAccessMask = VK_ACCESS_NONE;
	memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	memory_barrier.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memory_barrier.image = image_ptr->vk_image;
	memory_barrier.subresourceRange = image_subresource_range_default;

	VkPipelineStageFlags source_stage = 0;
	VkPipelineStageFlags destination_stage = 0;

	switch (image_layout_transition) {
		case IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_GENERAL:

			memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

			memory_barrier.srcAccessMask = VK_ACCESS_NONE;
			memory_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		
			source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

			break;
		case IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_TRANSFER_DST:

			memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			memory_barrier.srcAccessMask = VK_ACCESS_NONE;
			memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
			source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			break;
		case IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_SHADER_READ_ONLY:

			memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			break;
		case IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_GENERAL:

			memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

			memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			break;
	}

	if (memory_barrier.oldLayout != image_ptr->layout) {
		return;
	}

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0,
			0, NULL,
			0, NULL,
			1, &memory_barrier);

	vkEndCommandBuffer(command_buffer);
	submit_command_buffers_async(queue, 1, &command_buffer);
	vkFreeCommandBuffers(image_ptr->device, command_pool, 1, &command_buffer);

	image_ptr->layout = memory_barrier.newLayout;
}

void create_sampler(physical_device_t physical_device, VkDevice device, VkSampler *sampler_ptr) {

	static const VkFilter filter = VK_FILTER_NEAREST;
	static const VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkSamplerCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.magFilter = filter;
	create_info.minFilter = filter;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	create_info.addressModeU = address_mode;
	create_info.addressModeV = address_mode;
	create_info.addressModeW = address_mode;
	create_info.mipLodBias = 0.0F;
	create_info.anisotropyEnable = VK_TRUE;
	create_info.maxAnisotropy = physical_device.properties.limits.maxSamplerAnisotropy;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	create_info.minLod = 0.0F;
	create_info.maxLod = 0.0F;
	create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	create_info.unnormalizedCoordinates = VK_FALSE;

	vkCreateSampler(device, &create_info, NULL, sampler_ptr);
}

VkBufferImageCopy2 make_buffer_image_copy(VkOffset2D image_offset, VkExtent2D image_extent) {

	VkBufferImageCopy2 copy_region = { 0 };
	copy_region.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
	copy_region.pNext = NULL;
	copy_region.bufferOffset = 0;
	copy_region.bufferRowLength = 0;
	copy_region.bufferImageHeight = 0;
	copy_region.imageSubresource = image_subresource_layers_default;
	copy_region.imageOffset.x = image_offset.x;
	copy_region.imageOffset.y = image_offset.y;
	copy_region.imageOffset.z = 0;
	copy_region.imageExtent.width = image_extent.width;
	copy_region.imageExtent.height = image_extent.height;
	copy_region.imageExtent.depth = 1;

	return copy_region;
}
