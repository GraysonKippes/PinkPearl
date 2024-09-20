#include "Descriptor.h"

#include <stdlib.h>

#include "log/Logger.h"

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
