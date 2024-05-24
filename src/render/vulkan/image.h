#ifndef IMAGE_H
#define IMAGE_H

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "math/extent.h"

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

	uint32_t numArrayLayers;
	VkFormat format;
	VkImageLayout layout;

	VkImage vkImage;
	VkImageView vkImageView;

	VkDevice device;
	VkDeviceMemory memory;

} image_t;

image_t create_image(const image_create_info_t image_create_info);
bool destroy_image(image_t *const image_ptr);

void create_sampler(physical_device_t physical_device, VkDevice device, VkSampler *sampler_ptr);

#endif	// IMAGE_H
