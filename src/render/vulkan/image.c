#include "image.h"

static const VkImageSubresourceRange image_subresource_range_default = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseMipLevel = 0,
	.levelCount = 1,
	.baseArrayLayer = 0,
	.layerCount = VK_REMAINING_ARRAY_LAYERS
};

static const VkImageSubresourceLayers image_subresource_layers_default = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.mipLevel = 0,
	.baseArrayLayer = 0,
	.layerCount = VK_REMAINING_ARRAY_LAYERS
};

image_t create_image(const image_create_info_t image_create_info) {
	
	image_t image = { 0 };
	image.numArrayLayers = image_create_info.num_image_array_layers;
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
		.arrayLayers = image.numArrayLayers,
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
	
	vkCreateImage(image.device, &vk_image_create_info, NULL, &image.vkImage);

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(image.device, image.vkImage, &memory_requirements);

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
		.image = image.vkImage,
		.memory = image.memory,
		.memoryOffset = 0
	};

	vkBindImageMemory2(image.device, 1, &image_bind_info);

	const VkImageViewCreateInfo image_view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = image.vkImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		.format = image.format,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange = image_subresource_range_default
	};

	vkCreateImageView(image.device, &image_view_create_info, NULL, &image.vkImageView);

	return image;
}

bool destroy_image(image_t *const image_ptr) {
	
	if (image_ptr == NULL) {
		return false;
	}
	
	vkDestroyImage(image_ptr->device, image_ptr->vkImage, NULL);
	image_ptr->vkImage = VK_NULL_HANDLE;
	
	vkDestroyImageView(image_ptr->device, image_ptr->vkImageView, NULL);
	image_ptr->vkImageView = VK_NULL_HANDLE;
	
	vkFreeMemory(image_ptr->device, image_ptr->memory, NULL);
	image_ptr->memory = VK_NULL_HANDLE;
	
	image_ptr->numArrayLayers = 0;
	image_ptr->format = VK_FORMAT_UNDEFINED;
	image_ptr->layout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_ptr->device = VK_NULL_HANDLE;
	
	return true;
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

