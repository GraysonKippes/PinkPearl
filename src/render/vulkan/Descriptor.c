#include "Descriptor.h"

#include <stdlib.h>

#include "log/Logger.h"

// BINDLESS MANAGER

// All the types of descriptors that will be created by the bindless descriptor manager.
#define DESCRIPTOR_TYPE_COUNT 5
static const uint32_t descriptorTypeCount = DESCRIPTOR_TYPE_COUNT;
static const VkDescriptorType descriptorTypes[DESCRIPTOR_TYPE_COUNT] = {
	VK_DESCRIPTOR_TYPE_SAMPLER,
	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
};

static const uint32_t maxDescriptorCounts[DESCRIPTOR_TYPE_COUNT] = {
	4,		// Sampler descriptor count
	4096,	// Sampled image descriptor count
	256,	// Storage image descriptor count
	16,		// Uniform buffer descriptor count
	16		// Storage buffer descriptor count
};

static uint32_t descriptorCounts[DESCRIPTOR_TYPE_COUNT] = {
	0,		// Sampler descriptor count
	0,      // Sampled image descriptor count
	0,      // Storage image descriptor count
	0,      // Uniform buffer descriptor count
	0       // Storage buffer descriptor count
};

typedef enum DescriptorTypeBinding {
	DESCRIPTOR_BINDING_SAMPLER 			= 0,
	DESCRIPTOR_BINDING_SAMPLED_IMAGE 	= 1,
	DESCRIPTOR_BINDING_STORAGE_IMAGE 	= 2,
	DESCRIPTOR_BINDING_UNIFORM_BUFFER 	= 3,
	DESCRIPTOR_BINDING_STORAGE_BUFFER 	= 4
} DescriptorTypeBinding;

const uint32_t descriptorHandleInvalid = DESCRIPTOR_HANDLE_INVALID;

VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
VkDescriptorPool globalDescriptorPool = VK_NULL_HANDLE;
VkDescriptorSet globalDescriptorSet = VK_NULL_HANDLE;

void initDescriptorManager(const VkDevice vkDevice) {
	
	// Create global descriptor set layout.
	
	// Make the bindings for each descriptor type.
	VkDescriptorSetLayoutBinding bindings[DESCRIPTOR_TYPE_COUNT];
	VkDescriptorBindingFlags bindingFlags[DESCRIPTOR_TYPE_COUNT];
	for (uint32_t i = 0; i < descriptorTypeCount; ++i) {
		bindings[i] = (VkDescriptorSetLayoutBinding){
			.binding = i,
			.descriptorType = descriptorTypes[i],
			.descriptorCount = maxDescriptorCounts[i],
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		};
		bindingFlags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
	}
	
	const VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.pNext = nullptr,
		.bindingCount = descriptorTypeCount,
		.pBindingFlags = bindingFlags
	};

	const VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &bindingFlagsInfo,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount = descriptorTypeCount,
		.pBindings = bindings
	};
	
	const VkResult layoutCreateResult = vkCreateDescriptorSetLayout(vkDevice, &layoutCreateInfo, nullptr, &globalDescriptorSetLayout);
	if (layoutCreateResult != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error initializing descriptor manager: global descriptor set layout creation failed (error code: %i).", layoutCreateResult);
		return;
	}
	
	// Create global descriptor pool.
	
	VkDescriptorPoolSize poolSizes[DESCRIPTOR_TYPE_COUNT];
	for (uint32_t i = 0; i < descriptorTypeCount; ++i) {
		poolSizes[i] = (VkDescriptorPoolSize){
			.type = descriptorTypes[i],
			.descriptorCount = maxDescriptorCounts[i]
		};
	}
	
	const VkDescriptorPoolCreateInfo poolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		.maxSets = 1,
		.poolSizeCount = descriptorTypeCount,
		.pPoolSizes = poolSizes
	};
	
	const VkResult poolCreateResult = vkCreateDescriptorPool(vkDevice, &poolCreateInfo, nullptr, &globalDescriptorPool);
	if (poolCreateResult != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error initializing descriptor manager: global descriptor pool creation failed (error code: %i).", poolCreateResult);
		return;
	}
	
	// Allocate descriptor set.
	
	const VkDescriptorSetAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = globalDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &globalDescriptorSetLayout
	};

	const VkResult allocateResult = vkAllocateDescriptorSets(vkDevice, &allocateInfo, &globalDescriptorSet);
	if (allocateResult != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error initializing descriptor manager: global descriptor set allocation failed (error code: %i).", allocateResult);
	}
}

void terminateDescriptorManager(const VkDevice vkDevice) {
	vkDestroyDescriptorPool(vkDevice, globalDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vkDevice, globalDescriptorSetLayout, nullptr);
	globalDescriptorPool = VK_NULL_HANDLE;
	globalDescriptorSetLayout = VK_NULL_HANDLE;
}

uint32_t uploadSampler(const VkDevice vkDevice, const Sampler sampler) {
	
	static const uint32_t descriptorBinding = DESCRIPTOR_BINDING_SAMPLER;
	
	const uint32_t handle = descriptorCounts[descriptorBinding];
	if (handle >= maxDescriptorCounts[descriptorBinding]) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading sampler descriptor: no sampler descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorCounts[descriptorBinding] += 1;
	
	const VkDescriptorImageInfo descriptorImageInfo = {
		.sampler = sampler.vkSampler,
		.imageView = VK_NULL_HANDLE,
		.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = descriptorBinding,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[descriptorBinding],
		.descriptorCount = 1,
		.pBufferInfo = nullptr,
		.pImageInfo = &descriptorImageInfo,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadSampledImage(const VkDevice vkDevice, const Image image) {
	
	static const uint32_t descriptorBinding = DESCRIPTOR_BINDING_SAMPLED_IMAGE;
	
	const uint32_t handle = descriptorCounts[descriptorBinding];
	if (handle >= maxDescriptorCounts[descriptorBinding]) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading sampled image descriptor: no sampled image descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorCounts[descriptorBinding] += 1;
	
	const VkDescriptorImageInfo descriptorImageInfo = {
		.sampler = VK_NULL_HANDLE,
		.imageView = image.vkImageView,
		.imageLayout = image.usage.imageLayout
	};
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = descriptorBinding,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[descriptorBinding],
		.descriptorCount = 1,
		.pBufferInfo = nullptr,
		.pImageInfo = &descriptorImageInfo,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadStorageImage(const VkDevice vkDevice, const Image image) {
	
	static const uint32_t descriptorBinding = DESCRIPTOR_BINDING_STORAGE_IMAGE;
	
	const uint32_t handle = descriptorCounts[descriptorBinding];
	if (handle >= maxDescriptorCounts[descriptorBinding]) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading storage image descriptor: no storage image descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorCounts[descriptorBinding] += 1;
	
	const VkDescriptorImageInfo descriptorImageInfo = {
		.sampler = VK_NULL_HANDLE,
		.imageView = image.vkImageView,
		.imageLayout = image.usage.imageLayout
	};
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = descriptorBinding,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[descriptorBinding],
		.descriptorCount = 1,
		.pBufferInfo = nullptr,
		.pImageInfo = &descriptorImageInfo,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadUniformBuffer(const VkDevice vkDevice, const BufferPartition bufferPartition, const uint32_t partitionIndex) {
	
	static const uint32_t descriptorBinding = DESCRIPTOR_BINDING_UNIFORM_BUFFER;
	
	const uint32_t handle = descriptorCounts[descriptorBinding];
	if (handle >= maxDescriptorCounts[descriptorBinding]) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading uniform buffer descriptor: no uniform buffer descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorCounts[descriptorBinding] += 1;
	
	const VkDescriptorBufferInfo descriptorBufferInfo = buffer_partition_descriptor_info(bufferPartition, partitionIndex);
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = descriptorBinding,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[descriptorBinding],
		.descriptorCount = 1,
		.pBufferInfo = &descriptorBufferInfo,
		.pImageInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadUniformBuffer2(const VkDevice vkDevice, const BufferSubrange bufferSubrange) {
	
	static const uint32_t descriptorBinding = DESCRIPTOR_BINDING_UNIFORM_BUFFER;
	
	const uint32_t handle = descriptorCounts[descriptorBinding];
	if (handle >= maxDescriptorCounts[descriptorBinding]) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading uniform buffer descriptor: no uniform buffer descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorCounts[descriptorBinding] += 1;
	
	const VkDescriptorBufferInfo descriptorBufferInfo = makeDescriptorBufferInfo2(bufferSubrange);
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = descriptorBinding,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[descriptorBinding],
		.descriptorCount = 1,
		.pBufferInfo = &descriptorBufferInfo,
		.pImageInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadStorageBuffer(const VkDevice vkDevice, const BufferPartition bufferPartition, const uint32_t partitionIndex) {
	
	static const uint32_t descriptorBinding = DESCRIPTOR_BINDING_STORAGE_BUFFER;
	
	const uint32_t handle = descriptorCounts[descriptorBinding];
	if (handle >= maxDescriptorCounts[descriptorBinding]) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading storage buffer descriptor: no storage buffer descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorCounts[descriptorBinding] += 1;
	
	const VkDescriptorBufferInfo descriptorBufferInfo = buffer_partition_descriptor_info(bufferPartition, partitionIndex);
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = descriptorBinding,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[descriptorBinding],
		.descriptorCount = 1,
		.pBufferInfo = &descriptorBufferInfo,
		.pImageInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadStorageBuffer2(const VkDevice vkDevice, const BufferSubrange bufferSubrange) {
	
	static const uint32_t descriptorBinding = DESCRIPTOR_BINDING_STORAGE_BUFFER;
	
	const uint32_t handle = descriptorCounts[descriptorBinding];
	if (handle >= maxDescriptorCounts[descriptorBinding]) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading storage buffer descriptor: no storage buffer descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorCounts[descriptorBinding] += 1;
	
	const VkDescriptorBufferInfo descriptorBufferInfo = makeDescriptorBufferInfo2(bufferSubrange);
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = descriptorBinding,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[descriptorBinding],
		.descriptorCount = 1,
		.pBufferInfo = &descriptorBufferInfo,
		.pImageInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}