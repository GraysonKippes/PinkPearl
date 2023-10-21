#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <stdint.h>

#include <vulkan/vulkan.h>

/* -- TYPE DEFINITIONS -- */

// Condensed form of struct VkDescriptorSetLayoutBinding.
typedef struct descriptor_binding_t {

	VkDescriptorType m_type;
	uint32_t m_count;
	VkShaderStageFlags m_stages;

} descriptor_binding_t;

typedef struct descriptor_layout_t {
	
	// This can be used both for descriptor bindings and for sizes for descriptor pools.
	uint32_t m_num_bindings;
	descriptor_binding_t *m_bindings;

} descriptor_layout_t;

typedef struct descriptor_pool_t {

	VkDescriptorPool m_handle;
	VkDescriptorSetLayout m_layout;
} descriptor_pool_t;



/* -- GLOBAL CONST DEFINITIONS -- */

extern const descriptor_layout_t graphics_descriptor_layout;

extern const descriptor_layout_t compute_matrices_layout;



/* -- FUNCTION DECLARATIONS -- */

void create_descriptor_set_layout(VkDevice logical_device, descriptor_layout_t descriptor_layout, VkDescriptorSetLayout *descriptor_set_layout_ptr);

void create_descriptor_pool(VkDevice logical_device, uint32_t max_sets, descriptor_layout_t descriptor_layout, VkDescriptorPool *descriptor_pool_ptr);

void allocate_descriptor_sets(VkDevice logical_device, descriptor_pool_t descriptor_pool, uint32_t num_descriptor_sets, VkDescriptorSet *descriptor_sets);

#endif	// DESCRIPTOR_H
