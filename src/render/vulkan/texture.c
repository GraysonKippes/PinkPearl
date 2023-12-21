#include "texture.h"

#include <stdint.h>
#include <stdlib.h>

#include "buffer.h"
#include "command_buffer.h"

static const VkImageType image_type_default = VK_IMAGE_TYPE_2D;

static const VkFormat image_format_default = VK_FORMAT_R8G8B8A8_SRGB;

texture_t create_texture(VkDevice device, animation_set_t animation_set) {

	texture_t texture = { 0 };

	texture.format = image_format_default;
	texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	texture.device = device;

	texture.images = calloc(animation_set.num_animations, sizeof(VkImage));

	for (uint32_t i = 0; i < animation_set.num_animations; ++i) {

		VkImageCreateInfo image_create_info = { 0 };
		image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_create_info.pNext = NULL;
		image_create_info.flags = 0;
		image_create_info.imageType = image_type_default;
		image_create_info.format = texture.format;
		image_create_info.extent.width = animation_set.cell_extent.width;
		image_create_info.extent.height = animation_set.cell_extent.length;
		image_create_info.extent.depth = 1;
		image_create_info.mipLevels = 1;
		image_create_info.arrayLayers = animation_set.animations[i].num_frames;
		image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		const VkResult image_create_result = vkCreateImage(device, &image_create_info, NULL, (texture.images + i));
		if (image_create_result != VK_SUCCESS) {
			free(texture.images);
			texture.images = NULL;
			return texture;
		}
	}




	return texture;
}

void destroy_texture(texture_t texture) {

	for (uint32_t i = 0; i < texture.num_images; ++i) {
		vkDestroyImage(texture.device, texture.images[i], NULL);
		vkDestroyImageView(texture.device, texture.image_views[i], NULL);
	}

	vkFreeMemory(texture.device, texture.memory, NULL);

	free(texture.images);
	free(texture.image_views);
	free(texture.animation_cycles);
}
