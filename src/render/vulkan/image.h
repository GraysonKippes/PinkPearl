#ifndef IMAGE_H
#define IMAGE_H

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "render/stb/image_data.h"
#include "util/extent.h"

#include "memory.h"
#include "physical_device.h"
#include "queue.h"

typedef struct image_create_info_t {
	
	uint32_t num_image_array_layers;
	extent_t image_dimensions;
	
	VkFormat format;
	VkImageUsageFlags usage;
	queue_family_set_t queue_family_set;
	memory_type_index_t memory_type_index;
	
	VkPhysicalDevice physical_device;
	VkDevice device;
	
} image_create_info_t;

typedef struct image_t {

	uint32_t num_array_layers;
	VkFormat format;
	VkImageLayout layout;

	VkImage vk_image;
	VkImageView vk_image_view;

	VkDevice device;
	VkDeviceMemory memory;

} image_t;

typedef enum image_layout_transition_t {
	IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_GENERAL,
	IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_TRANSFER_DST,
	IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_SHADER_READ_ONLY,
	IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_GENERAL
} image_layout_transition_t;



image_t create_image(const image_create_info_t image_create_info);
bool destroy_image(image_t *const image_ptr);

void create_sampler(physical_device_t physical_device, VkDevice device, VkSampler *sampler_ptr);

VkBufferImageCopy2 make_buffer_image_copy(VkOffset2D image_offset, VkExtent2D image_extent);

#endif	// IMAGE_H
