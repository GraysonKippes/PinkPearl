#include "Descriptor.h"

#include <stdlib.h>

#include "log/Logger.h"

// BINDLESS MANAGER

// All the types of descriptors that will be created by the bindless descriptor manager.
#define DESCRIPTOR_TYPE_COUNT 4
static const uint32_t descriptorTypeCount = DESCRIPTOR_TYPE_COUNT;
static const VkDescriptorType descriptorTypes[DESCRIPTOR_TYPE_COUNT] = {
	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
};

typedef enum DescriptorTypeBinding {
	DESCRIPTOR_BINDING_SAMPLED_IMAGE = 0,
	DESCRIPTOR_BINDING_STORAGE_IMAGE = 1,
	DESCRIPTOR_BINDING_UNIFORM_BUFFER = 2,
	DESCRIPTOR_BINDING_STORAGE_BUFFER = 3
} DescriptorTypeBinding;

// The number of descriptors to allocate for each type.
#define MAX_DESCRIPTOR_COUNT 4096
static const uint32_t maxDescriptorCount = MAX_DESCRIPTOR_COUNT;

static uint32_t descriptorSampledImageCount = 0;
static uint32_t descriptorStorageImageCount = 0;
static uint32_t descriptorUniformBufferCount = 0;
static uint32_t descriptorStorageBufferCount = 0;

const uint32_t descriptorHandleInvalid = UINT32_MAX;

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
			.descriptorCount = maxDescriptorCount,
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
			.descriptorCount = maxDescriptorCount
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

uint32_t uploadSampledImage(const VkDevice vkDevice, const Image image) {
	
	const uint32_t handle = descriptorSampledImageCount;
	if (handle >= maxDescriptorCount) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading sampled image descriptor: no sampled image descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorSampledImageCount += 1;
	
	const VkDescriptorImageInfo descriptorImageInfo = {
		.sampler = VK_NULL_HANDLE,
		.imageView = image.vkImageView,
		.imageLayout = image.usage.imageLayout
	};
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = (uint32_t)DESCRIPTOR_BINDING_SAMPLED_IMAGE,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[DESCRIPTOR_BINDING_SAMPLED_IMAGE],
		.descriptorCount = 1,
		.pBufferInfo = nullptr,
		.pImageInfo = &descriptorImageInfo,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadStorageImage(const VkDevice vkDevice, const Image image) {
	
	const uint32_t handle = descriptorStorageImageCount;
	if (handle >= maxDescriptorCount) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading storage image descriptor: no storage image descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorStorageImageCount += 1;
	
	const VkDescriptorImageInfo descriptorImageInfo = {
		.sampler = VK_NULL_HANDLE,
		.imageView = image.vkImageView,
		.imageLayout = image.usage.imageLayout
	};
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = (uint32_t)DESCRIPTOR_BINDING_STORAGE_IMAGE,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[DESCRIPTOR_BINDING_STORAGE_IMAGE],
		.descriptorCount = 1,
		.pBufferInfo = nullptr,
		.pImageInfo = &descriptorImageInfo,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadUniformBuffer(const VkDevice vkDevice, const BufferPartition bufferPartition, const uint32_t partitionIndex) {
	
	const uint32_t handle = descriptorUniformBufferCount;
	if (handle >= maxDescriptorCount) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading uniform buffer descriptor: no uniform buffer descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorUniformBufferCount += 1;
	
	const VkDescriptorBufferInfo descriptorBufferInfo = buffer_partition_descriptor_info(bufferPartition, partitionIndex);
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = (uint32_t)DESCRIPTOR_BINDING_UNIFORM_BUFFER,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[DESCRIPTOR_BINDING_UNIFORM_BUFFER],
		.descriptorCount = 1,
		.pBufferInfo = &descriptorBufferInfo,
		.pImageInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadUniformBuffer2(const VkDevice vkDevice, const BufferSubrange bufferSubrange) {
	
	const uint32_t handle = descriptorUniformBufferCount;
	if (handle >= maxDescriptorCount) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading uniform buffer descriptor: no uniform buffer descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorUniformBufferCount += 1;
	
	const VkDescriptorBufferInfo descriptorBufferInfo = makeDescriptorBufferInfo2(bufferSubrange);
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = (uint32_t)DESCRIPTOR_BINDING_UNIFORM_BUFFER,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[DESCRIPTOR_BINDING_UNIFORM_BUFFER],
		.descriptorCount = 1,
		.pBufferInfo = &descriptorBufferInfo,
		.pImageInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadStorageBuffer(const VkDevice vkDevice, const BufferPartition bufferPartition, const uint32_t partitionIndex) {
	
	const uint32_t handle = descriptorStorageBufferCount;
	if (handle >= maxDescriptorCount) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading storage buffer descriptor: no storage buffer descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorStorageBufferCount += 1;
	
	const VkDescriptorBufferInfo descriptorBufferInfo = buffer_partition_descriptor_info(bufferPartition, partitionIndex);
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = (uint32_t)DESCRIPTOR_BINDING_STORAGE_BUFFER,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[DESCRIPTOR_BINDING_STORAGE_BUFFER],
		.descriptorCount = 1,
		.pBufferInfo = &descriptorBufferInfo,
		.pImageInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}

uint32_t uploadStorageBuffer2(const VkDevice vkDevice, const BufferSubrange bufferSubrange) {
	
	const uint32_t handle = descriptorStorageBufferCount;
	if (handle >= maxDescriptorCount) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error uploading storage buffer descriptor: no storage buffer descriptors available.");
		return descriptorHandleInvalid;
	}
	descriptorStorageBufferCount += 1;
	
	const VkDescriptorBufferInfo descriptorBufferInfo = makeDescriptorBufferInfo2(bufferSubrange);
	
	const VkWriteDescriptorSet writeDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = globalDescriptorSet,
		.dstBinding = (uint32_t)DESCRIPTOR_BINDING_STORAGE_BUFFER,
		.dstArrayElement = handle,
		.descriptorType = descriptorTypes[DESCRIPTOR_BINDING_STORAGE_BUFFER],
		.descriptorCount = 1,
		.pBufferInfo = &descriptorBufferInfo,
		.pImageInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	
	vkUpdateDescriptorSets(vkDevice, 1, &writeDescriptorSet, 0, nullptr);
	
	return handle;
}





// OLD

DescriptorSet allocateDescriptorSet(const DescriptorPool descriptorPool) {
	
	DescriptorSet descriptorSet = {
		.vkDescriptorSet = VK_NULL_HANDLE,
		.vkDevice = VK_NULL_HANDLE,
		.bound = false,
		.pendingWriteCount = 0,
		.pPendingWrites = nullptr
	};
	
	VkDescriptorSetAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool.handle,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptorPool.layout
	};
	vkAllocateDescriptorSets(descriptorPool.vkDevice, &allocateInfo, &descriptorSet.vkDescriptorSet);
	
	descriptorSet.vkDevice = descriptorPool.vkDevice;
	descriptorSet.pPendingWrites = malloc(8 * sizeof(VkWriteDescriptorSet));
	
	return descriptorSet;
}

void descriptorSetPushWrite(DescriptorSet *const pDescriptorSet, const VkWriteDescriptorSet descriptorSetWrite) {
	if (!pDescriptorSet) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error pushing descriptor set write: null pointer(s) detected.");
		return;
	}
	
	if (pDescriptorSet->bound) {
		if (pDescriptorSet->pendingWriteCount >= 8) {
			logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error pushing descriptor set write: maximum number of writes already pushed.");
			return;
		}
		pDescriptorSet->pPendingWrites[pDescriptorSet->pendingWriteCount] = descriptorSetWrite;
		pDescriptorSet->pendingWriteCount += 1;
	} else {
		vkUpdateDescriptorSets(pDescriptorSet->vkDevice, 1, &descriptorSetWrite, 0, nullptr);
	}
}



void create_descriptor_set_layout(VkDevice device, DescriptorSetLayout descriptor_layout, VkDescriptorSetLayout *pDescriptorSetLayout) {

	VkDescriptorSetLayoutBinding bindings[descriptor_layout.num_bindings];
	VkDescriptorBindingFlags bindingFlags[descriptor_layout.num_bindings];
	for (uint32_t i = 0; i < descriptor_layout.num_bindings; ++i) {
		bindings[i] = (VkDescriptorSetLayoutBinding){
			.binding = i,
			.descriptorType = descriptor_layout.bindings[i].type,
			.descriptorCount = descriptor_layout.bindings[i].count,
			.stageFlags = descriptor_layout.bindings[i].stages,
			.pImmutableSamplers = nullptr
		};
		bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
	}

	const VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.pNext = nullptr,
		.bindingCount = descriptor_layout.num_bindings,
		.pBindingFlags = bindingFlags
	};

	const VkDescriptorSetLayoutCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &bindingFlagsInfo,
		.flags = 0,
		.bindingCount = descriptor_layout.num_bindings,
		.pBindings = bindings
	};

	VkResult result = vkCreateDescriptorSetLayout(device, &createInfo, nullptr, pDescriptorSetLayout);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Descriptor set layout creation failed. (Error code: %i)", result);
	}
}

void create_descriptor_pool(VkDevice device, uint32_t max_sets, DescriptorSetLayout descriptor_layout, VkDescriptorPool *descriptor_pool_ptr) {

	VkDescriptorPoolSize pool_sizes[descriptor_layout.num_bindings];
	for (uint32_t i = 0; i < descriptor_layout.num_bindings; ++i) {
		pool_sizes[i].type = descriptor_layout.bindings[i].type;
		pool_sizes[i].descriptorCount = descriptor_layout.bindings[i].count;
	}

	VkDescriptorPoolCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	create_info.maxSets = max_sets;
	create_info.poolSizeCount = descriptor_layout.num_bindings;
	create_info.pPoolSizes = pool_sizes;

	VkResult result = vkCreateDescriptorPool(device, &create_info, nullptr, descriptor_pool_ptr);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_FATAL, "Descriptor pool creation failed. (Error code: %i)", result);
	}
}

void destroy_descriptor_pool(VkDevice device, DescriptorPool descriptor_pool) {
	vkDestroyDescriptorPool(device, descriptor_pool.handle, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptor_pool.layout, nullptr);
}

void allocate_descriptor_sets(VkDevice device, DescriptorPool descriptor_pool, uint32_t num_descriptor_sets, VkDescriptorSet *descriptor_sets) {

	VkDescriptorSetLayout *layouts = calloc(num_descriptor_sets, sizeof(VkDescriptorSetLayout));
	if (layouts == nullptr) {
		return;
	}

	for (uint32_t i = 0; i < num_descriptor_sets; ++i)
		layouts[i] = descriptor_pool.layout;

	VkDescriptorSetAllocateInfo allocate_info;
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = nullptr;
	allocate_info.descriptorPool = descriptor_pool.handle;
	allocate_info.descriptorSetCount = num_descriptor_sets;
	allocate_info.pSetLayouts = layouts;

	VkResult result = vkAllocateDescriptorSets(device, &allocate_info, descriptor_sets);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Descriptor set allocation failed. (Error code: %i)", result);
	}

	free(layouts);
}

VkDescriptorBufferInfo makeDescriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {

	VkDescriptorBufferInfo info = { 0 };
	info.buffer = buffer;
	info.offset = offset;
	info.range = range;

	return info;
}

VkDescriptorImageInfo makeDescriptorImageInfo(const Image image) {
	return (VkDescriptorImageInfo){
		.sampler = VK_NULL_HANDLE,
		.imageView = image.vkImageView,
		.imageLayout = image.usage.imageLayout
	};
}

VkDescriptorImageInfo makeDescriptorSamplerInfo(const Sampler sampler) {
	return (VkDescriptorImageInfo){
		.sampler = sampler.vkSampler,
		.imageView = VK_NULL_HANDLE,
		.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
}
