#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "image.h"

/* -- TYPE DEFINITIONS -- */

// Condensed form of struct VkDescriptorSetLayoutBinding.
typedef struct descriptor_binding_t {

	VkDescriptorType m_type;
	uint32_t m_count;
	VkShaderStageFlags m_stages;

} descriptor_binding_t;



// Can be used to create a VkDescriptorSetLayout.
typedef struct descriptor_layout_t {
	
	// This can be used both for descriptor bindings and for sizes for descriptor pools.
	uint32_t m_num_bindings;
	descriptor_binding_t *m_bindings;

} descriptor_layout_t;

typedef struct descriptor_pool_t {

	VkDescriptorPool m_handle;
	VkDescriptorSetLayout m_layout;

} descriptor_pool_t;



/* -- FUNCTION DECLARATIONS -- */

void create_descriptor_set_layout(VkDevice logical_device, descriptor_layout_t descriptor_layout, VkDescriptorSetLayout *descriptor_set_layout_ptr);

// TODO - potentially change this function to take VkDescriptorSetLayout instead.
void create_descriptor_pool(VkDevice logical_device, uint32_t max_sets, descriptor_layout_t descriptor_layout, VkDescriptorPool *descriptor_pool_ptr);

void destroy_descriptor_pool(VkDevice logical_device, descriptor_pool_t descriptor_pool);

void allocate_descriptor_sets(VkDevice logical_device, descriptor_pool_t descriptor_pool, uint32_t num_descriptor_sets, VkDescriptorSet *descriptor_sets);

// Convenience function. Makes and returns a VkDescriptorBufferInfo struct with the specified parameters.
VkDescriptorBufferInfo make_descriptor_buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

extern const VkSampler no_sampler;

// Convenience function. Makes and returns a VkDescriptorImageInfo struct with the specified parameters.
// Pass VK_NULL_HANDLE for the sampler if the image in question is not going to be sampled.
VkDescriptorImageInfo make_descriptor_image_info(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);

#endif	// DESCRIPTOR_H
