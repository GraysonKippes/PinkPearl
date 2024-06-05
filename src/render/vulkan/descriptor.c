#include "descriptor.h"

#include <stdlib.h>

#include "log/logging.h"



void create_descriptor_set_layout(VkDevice device, descriptor_layout_t descriptor_layout, VkDescriptorSetLayout *descriptor_set_layout_ptr) {

	VkDescriptorSetLayoutBinding *descriptor_bindings = nullptr;

	if (descriptor_layout.num_bindings > 0) {
		descriptor_bindings = calloc(descriptor_layout.num_bindings, sizeof(VkDescriptorSetLayoutBinding));
		if (descriptor_bindings == nullptr) {
			return;
		}
	}

	for (uint32_t i = 0; i < descriptor_layout.num_bindings; ++i) {
		descriptor_bindings[i].binding = i;
		descriptor_bindings[i].descriptorType = descriptor_layout.bindings[i].type;
		descriptor_bindings[i].descriptorCount = descriptor_layout.bindings[i].count;
		descriptor_bindings[i].stageFlags = descriptor_layout.bindings[i].stages;
	}

	VkDescriptorSetLayoutCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.bindingCount = descriptor_layout.num_bindings;
	create_info.pBindings = descriptor_bindings;

	VkResult result = vkCreateDescriptorSetLayout(device, &create_info, nullptr, descriptor_set_layout_ptr);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Descriptor set layout creation failed. (Error code: %i)", result);
	}

	if (descriptor_bindings != nullptr)
		free(descriptor_bindings);
}

void create_descriptor_pool(VkDevice device, uint32_t max_sets, descriptor_layout_t descriptor_layout, VkDescriptorPool *descriptor_pool_ptr) {

	VkDescriptorPoolSize *pool_sizes = calloc(descriptor_layout.num_bindings, sizeof(VkDescriptorPoolSize));
	if (pool_sizes == nullptr) {
		log_message(ERROR, "Allocation of descriptor pool sizes array failed.");
		return;
	}
	
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
		logf_message(FATAL, "Descriptor pool creation failed. (Error code: %i)", result);
	}

	free(pool_sizes);
}

void destroy_descriptor_pool(VkDevice device, descriptor_pool_t descriptor_pool) {
	vkDestroyDescriptorPool(device, descriptor_pool.handle, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptor_pool.layout, nullptr);
}

void allocate_descriptor_sets(VkDevice device, descriptor_pool_t descriptor_pool, uint32_t num_descriptor_sets, VkDescriptorSet *descriptor_sets) {

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
		logf_message(ERROR, "Descriptor set allocation failed. (Error code: %i)", result);
	}

	free(layouts);
}

VkDescriptorBufferInfo make_descriptor_buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {

	VkDescriptorBufferInfo info = { 0 };
	info.buffer = buffer;
	info.offset = offset;
	info.range = range;

	return info;
}

const VkSampler no_sampler = VK_NULL_HANDLE;

VkDescriptorImageInfo make_descriptor_image_info(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout) {

	VkDescriptorImageInfo info = { 0 };
	info.sampler = sampler;
	info.imageView = image_view;
	info.imageLayout = image_layout;

	return info;
}
