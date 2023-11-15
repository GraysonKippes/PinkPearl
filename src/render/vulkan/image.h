#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "physical_device.h"
#include "queue.h"
#include "render/stb/image_data.h"



typedef struct image_t {

	VkImage m_handle;
	VkFormat m_format;
	VkImageView m_view;
	VkImageLayout m_layout;

	VkDeviceMemory m_memory;

	// The device with which this image was created.
	VkDevice m_device;

} image_t;

typedef struct image_dimensions_t {
	uint32_t m_width;
	uint32_t m_height;
} image_dimensions_t;

typedef enum image_layout_transition_t {
	IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_GENERAL,
	IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_TRANSFER_DST,
	IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_SHADER_READ_ONLY,
	IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_GENERAL
} image_layout_transition_t;



extern const VkImageSubresourceLayers image_subresource_layers_default;

image_t create_image(VkPhysicalDevice physical_device, VkDevice device, image_dimensions_t dimensions, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties, queue_family_set_t queue_family_set);

void destroy_image(image_t image);

// Make sure the command pool has reset bit on.
void transition_image_layout(VkQueue queue, VkCommandPool command_pool, image_t *image_ptr, image_layout_transition_t image_layout_transition);

void create_sampler(physical_device_t physical_device, VkDevice device, VkSampler *sampler_ptr);

VkBufferImageCopy2 make_buffer_image_copy(VkOffset2D image_offset, VkExtent2D image_extent);

#endif	// IMAGE_H
