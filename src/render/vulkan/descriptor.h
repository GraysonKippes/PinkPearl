#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "image.h"

/* -- TYPE DEFINITIONS -- */

// Condensed form of struct VkDescriptorSetLayoutBinding.
typedef struct DescriptorBinding {

	VkDescriptorType type;
	uint32_t count;
	VkShaderStageFlags stages;

} DescriptorBinding;

// Can be used to create a VkDescriptorSetLayout.
typedef struct DescriptorSetLayout {
	
	// This can be used both for descriptor bindings and for sizes for descriptor pools.
	uint32_t num_bindings;
	DescriptorBinding *bindings;

} DescriptorSetLayout;

typedef struct DescriptorPool {

	VkDescriptorPool handle;
	VkDescriptorSetLayout layout;
	VkDevice vkDevice;

} DescriptorPool;

typedef struct DescriptorSet {
	
	VkDescriptorSet vkDescriptorSet;
	VkDevice vkDevice;
	
	bool bound;
	uint32_t pendingWriteCount;
	VkWriteDescriptorSet *pPendingWrites;
	
} DescriptorSet;

DescriptorSet allocateDescriptorSet(const DescriptorPool descriptorPool);

void descriptorSetPushWrite(DescriptorSet *const pDescriptorSet, const VkWriteDescriptorSet descriptorSetWrite);



extern const VkSampler no_sampler;

/* -- FUNCTION DECLARATIONS -- */

void create_descriptor_set_layout(VkDevice vkDevice, DescriptorSetLayout descriptorSetLayout, VkDescriptorSetLayout *pDescriptorSetLayout);

void create_descriptor_pool(VkDevice vkDevice, uint32_t max_sets, DescriptorSetLayout descriptor_layout, VkDescriptorPool *descriptor_pool_ptr);

void destroy_descriptor_pool(VkDevice vkDevice, DescriptorPool descriptorPool);

void allocate_descriptor_sets(VkDevice vkDevice, DescriptorPool descriptor_pool, uint32_t num_descriptor_sets, VkDescriptorSet *descriptor_sets);

// Convenience function. Makes and returns a VkDescriptorBufferInfo struct with the specified parameters.
VkDescriptorBufferInfo makeDescriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

// Convenience function. Makes and returns a VkDescriptorImageInfo struct with the specified parameters.
// Pass VK_NULL_HANDLE for the sampler if the image in question is not going to be sampled.
VkDescriptorImageInfo makeDescriptorImageInfo(const VkSampler sampler, const VkImageView imageView, const VkImageLayout imageLayout);

#endif	// DESCRIPTOR_H
