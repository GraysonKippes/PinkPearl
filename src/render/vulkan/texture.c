#include "texture.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "buffer.h"
#include "command_buffer.h"

texture_t make_null_texture(void) {
	return (texture_t){
		.num_images = 0,
		.images = NULL,
		.format = VK_FORMAT_UNDEFINED,
		.layout = VK_IMAGE_LAYOUT_UNDEFINED,
		.memory = VK_NULL_HANDLE,
		.device = VK_NULL_HANDLE
	};
}

bool destroy_texture(texture_t *restrict const texture_ptr) {

	if (texture_ptr == NULL) {
		return false;
	}

	if (texture_ptr->images == NULL) {
		return false;
	}

	for (uint32_t i = 0; i < texture_ptr->num_images; ++i) {
		vkDestroyImage(texture_ptr->device, texture_ptr->images[i].vk_image, NULL);
		vkDestroyImageView(texture_ptr->device, texture_ptr->images[i].vk_image_view, NULL);
	}
	free(texture_ptr->images);
	texture_ptr->num_images = 0;
	texture_ptr->images = NULL;

	texture_ptr->format = VK_FORMAT_UNDEFINED;
	texture_ptr->layout = VK_IMAGE_LAYOUT_UNDEFINED;

	vkFreeMemory(texture_ptr->device, texture_ptr->memory, NULL);
	texture_ptr->memory = VK_NULL_HANDLE;
	texture_ptr->device = VK_NULL_HANDLE;

	return true;
}

