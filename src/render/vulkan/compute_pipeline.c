#include "compute_pipeline.h"

#include <stdlib.h>

#include "shader.h"

#include "log/logging.h"

void create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, VkPipelineLayout *pipeline_layout_ptr) {

	VkPipelineLayoutCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = &descriptor_set_layout;
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = nullptr;

	vkCreatePipelineLayout(device, &create_info, nullptr, pipeline_layout_ptr);
}

ComputePipeline create_compute_pipeline(const VkDevice device, const descriptor_layout_t descriptor_layout, const char *const compute_shader_name) {
	
	shader_module_t compute_shader_module = create_shader_module(device, compute_shader_name);

	ComputePipeline compute_pipeline = { 0 };

	create_descriptor_set_layout(device, descriptor_layout, &compute_pipeline.descriptor_set_layout);
	create_pipeline_layout(device, compute_pipeline.descriptor_set_layout, &compute_pipeline.layout);
	// TODO - manage descriptor set memory better.
	create_descriptor_pool(device, 256, descriptor_layout, &compute_pipeline.descriptor_pool);

	const VkComputePipelineCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.layout = compute_pipeline.layout,
		.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.stage.module = compute_shader_module.module_handle,
		.stage.pName = "main"
	};

	const VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &create_info, nullptr, &compute_pipeline.handle);
	if (result != VK_SUCCESS) {
		logMsgF(FATAL, "Compute pipeline creation failed (error code: %i).", result);
	}
	compute_pipeline.device = device;

	destroy_shader_module(&compute_shader_module);
	return compute_pipeline;
}

bool destroy_compute_pipeline(ComputePipeline *const compute_pipeline_ptr) {

	if (compute_pipeline_ptr == nullptr) {
		return false;
	}

	vkDestroyPipeline(compute_pipeline_ptr->device, compute_pipeline_ptr->handle, nullptr);
	compute_pipeline_ptr->handle = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(compute_pipeline_ptr->device, compute_pipeline_ptr->layout, nullptr);
	compute_pipeline_ptr->layout = VK_NULL_HANDLE;

	vkDestroyDescriptorPool(compute_pipeline_ptr->device, compute_pipeline_ptr->descriptor_pool, nullptr);
	compute_pipeline_ptr->descriptor_pool = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(compute_pipeline_ptr->device, compute_pipeline_ptr->descriptor_set_layout, nullptr);
	compute_pipeline_ptr->descriptor_set_layout = VK_NULL_HANDLE;

	compute_pipeline_ptr->device = VK_NULL_HANDLE;

	return true;
}
