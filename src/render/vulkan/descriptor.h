#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "Buffer2.h"
#include "texture.h"

void initDescriptorManager(const VkDevice vkDevice);
void terminateDescriptorManager(const VkDevice vkDevice);

uint32_t uploadSampledImage(const VkDevice vkDevice, const Image image);

uint32_t uploadStorageImage(const VkDevice vkDevice, const Image image);

uint32_t uploadUniformBuffer(const VkDevice vkDevice, const BufferPartition bufferPartition, const uint32_t partitionIndex);
uint32_t uploadUniformBuffer2(const VkDevice vkDevice, const BufferSubrange bufferSubrange);

uint32_t uploadStorageBuffer(const VkDevice vkDevice, const BufferPartition bufferPartition, const uint32_t partitionIndex);
uint32_t uploadStorageBuffer2(const VkDevice vkDevice, const BufferSubrange bufferSubrange);

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

/* -- FUNCTION DECLARATIONS -- */

void create_descriptor_set_layout(VkDevice vkDevice, DescriptorSetLayout descriptorSetLayout, VkDescriptorSetLayout *pDescriptorSetLayout);

void create_descriptor_pool(VkDevice vkDevice, uint32_t max_sets, DescriptorSetLayout descriptor_layout, VkDescriptorPool *descriptor_pool_ptr);

void destroy_descriptor_pool(VkDevice vkDevice, DescriptorPool descriptorPool);

void allocate_descriptor_sets(VkDevice vkDevice, DescriptorPool descriptor_pool, uint32_t num_descriptor_sets, VkDescriptorSet *descriptor_sets);

// Returns a VkDescriptorBufferInfo struct with the specified parameters.
VkDescriptorBufferInfo makeDescriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

// Returns a VkDescriptorImageInfo struct with the view and layout of the specified image.
VkDescriptorImageInfo makeDescriptorImageInfo(const Image image);

// Returns a VkDescriptorImageInfo struct for the specified sampler.
VkDescriptorImageInfo makeDescriptorSamplerInfo(const Sampler sampler);

#endif	// DESCRIPTOR_H
