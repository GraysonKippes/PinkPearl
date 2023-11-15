#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "image.h"

/* -- TYPE DEFINITIONS -- */

// Condensed form of struct VkDescriptorSetLayoutBinding.
typedef struct descriptor_binding_t {

	VkDescriptorType type;
	uint32_t count;
	VkShaderStageFlags stages;

} descriptor_binding_t;



// Can be used to create a VkDescriptorSetLayout.
// TODO - change typename to `descriptor_set_layout_t`.
typedef struct descriptor_layout_t {
	
	// This can be used both for descriptor bindings and for sizes for descriptor pools.
	uint32_t num_bindings;
	descriptor_binding_t *bindings;

} descriptor_layout_t;

typedef struct descriptor_pool_t {

	VkDescriptorPool handle;
	VkDescriptorSetLayout layout;

} descriptor_pool_t;



/* -- FUNCTION DECLARATIONS -- */

void create_descriptor_set_layout(VkDevice device, descriptor_layout_t descriptor_layout, VkDescriptorSetLayout *descriptor_set_layout_ptr);

void create_descriptor_pool(VkDevice device, uint32_t max_sets, descriptor_layout_t descriptor_layout, VkDescriptorPool *descriptor_pool_ptr);

void destroy_descriptor_pool(VkDevice device, descriptor_pool_t descriptor_pool);

void allocate_descriptor_sets(VkDevice device, descriptor_pool_t descriptor_pool, uint32_t num_descriptor_sets, VkDescriptorSet *descriptor_sets);

// Convenience function. Makes and returns a VkDescriptorBufferInfo struct with the specified parameters.
VkDescriptorBufferInfo make_descriptor_buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

extern const VkSampler no_sampler;

// Convenience function. Makes and returns a VkDescriptorImageInfo struct with the specified parameters.
// Pass VK_NULL_HANDLE for the sampler if the image in question is not going to be sampled.
VkDescriptorImageInfo make_descriptor_image_info(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);

#endif	// DESCRIPTOR_H
