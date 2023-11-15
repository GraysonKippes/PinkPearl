#include "image.h"

#include <stdint.h>

#include "command_buffer.h"

static const VkImageType image_type_default = VK_IMAGE_TYPE_2D;

static const VkFormat image_format_default = VK_FORMAT_R8G8B8A8_SRGB;
//static const VkFormat image_format_0 = VK_FORMAT_R8G8B8A8_UINT;
static const VkFormat image_format_0 = VK_FORMAT_R8G8B8A8_SRGB;

static const VkImageSubresourceRange image_subresource_range_default = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseMipLevel = 0,
	.levelCount = 1,
	.baseArrayLayer = 0,
	.layerCount = 1
};

const VkImageSubresourceLayers image_subresource_layers_default = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.mipLevel = 0,
	.baseArrayLayer = 0,
	.layerCount = 1
};



image_t create_image(VkPhysicalDevice physical_device, VkDevice device, image_dimensions_t dimensions, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties, queue_family_set_t queue_family_set) {
	
	image_t image = { 0 };

	image.format = format;
	image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	image.device = device;

	VkImageCreateInfo image_create_info = { 0 };
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.pNext = NULL;
	image_create_info.flags = 0;
	image_create_info.imageType = image_type_default;
	image_create_info.format = image.format;
	image_create_info.extent.width = dimensions.width;
	image_create_info.extent.height = dimensions.height;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage = usage;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (is_queue_family_set_null(queue_family_set)) {
		image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_create_info.queueFamilyIndexCount = 0;
		image_create_info.pQueueFamilyIndices = NULL;
	}
	else {
		image_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		image_create_info.queueFamilyIndexCount = queue_family_set.num_queue_families;
		image_create_info.pQueueFamilyIndices = queue_family_set.queue_families;
	}
	
	vkCreateImage(device, &image_create_info, NULL, &image.handle);

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, image.handle, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = memory_requirements.size;
	if(!find_physical_device_memory_type(physical_device, memory_requirements.memoryTypeBits, memory_properties, &allocate_info.memoryTypeIndex)) {
		// TODO - error handling
	}

	vkAllocateMemory(device, &allocate_info, NULL, &image.memory);

	VkBindImageMemoryInfo bind_info = { 0 };
	bind_info.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
	bind_info.pNext = NULL;
	bind_info.image = image.handle;
	bind_info.memory = image.memory;
	bind_info.memoryOffset = 0;

	vkBindImageMemory2(device, 1, &bind_info);

	VkImageViewCreateInfo image_view_create_info = { 0 };
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.pNext = NULL;
	image_view_create_info.flags = 0;
	image_view_create_info.image = image.handle;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format = image.format;
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.subresourceRange = image_subresource_range_default;

	vkCreateImageView(device, &image_view_create_info, NULL, &image.view);

	return image;
}

void destroy_image(image_t image) {
	vkDestroyImage(image.device, image.handle, NULL);
	vkDestroyImageView(image.device, image.view, NULL);
	vkFreeMemory(image.device, image.memory, NULL);
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
	memory_barrier.image = image_ptr->handle;
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
