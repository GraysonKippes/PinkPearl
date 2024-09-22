#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "Buffer2.h"
#include "texture.h"

// Initializes the bindless descriptor manager.
// This function must be called before any upload resource functions can be called.
void initDescriptorManager(const VkDevice vkDevice);

// Terminates the bindless descriptor manager.
void terminateDescriptorManager(const VkDevice vkDevice);

// Uploads a sampler to the sampler descriptor array.
// Returns a handle to the resource to be used inside shaders.
uint32_t uploadSampler(const VkDevice vkDevice, const Sampler sampler);

// Uploads a sampled image to the sampled image descriptor array.
// Returns a handle to the resource to be used inside shaders.
uint32_t uploadSampledImage(const VkDevice vkDevice, const Image image);

// Uploads a storage image to the storage image descriptor array.
// Returns a handle to the resource to be used inside shaders.
uint32_t uploadStorageImage(const VkDevice vkDevice, const Image image);

uint32_t uploadUniformBuffer(const VkDevice vkDevice, const BufferPartition bufferPartition, const uint32_t partitionIndex);
uint32_t uploadUniformBuffer2(const VkDevice vkDevice, const BufferSubrange bufferSubrange);

uint32_t uploadStorageBuffer(const VkDevice vkDevice, const BufferPartition bufferPartition, const uint32_t partitionIndex);
uint32_t uploadStorageBuffer2(const VkDevice vkDevice, const BufferSubrange bufferSubrange);

// The descriptor set layout for the bindless descriptor set.
extern VkDescriptorSetLayout globalDescriptorSetLayout;

// The memory pool for the bindless descriptor set.
extern VkDescriptorPool globalDescriptorPool;

// The bindless descriptor set.
extern VkDescriptorSet globalDescriptorSet;

#define DESCRIPTOR_HANDLE_INVALID UINT32_MAX
extern const uint32_t descriptorHandleInvalid;

#endif	// DESCRIPTOR_H
