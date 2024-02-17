#include "compute_pipeline.h"

#include <stdlib.h>

#include "shader.h"

#include "log/logging.h"

void create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, VkPipelineLayout *pipeline_layout_ptr) {

	VkPipelineLayoutCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = &descriptor_set_layout;
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = NULL;

	vkCreatePipelineLayout(device, &create_info, NULL, pipeline_layout_ptr);
}

compute_pipeline_t create_compute_pipeline(const VkDevice device, const descriptor_layout_t descriptor_layout, const char *const compute_shader_name) {
	
	VkShaderModule compute_shader;
	create_shader_module(device, compute_shader_name, &compute_shader);

	compute_pipeline_t compute_pipeline = { 0 };
	compute_pipeline.device = device;

	create_descriptor_set_layout(device, descriptor_layout, &compute_pipeline.descriptor_set_layout);
	create_pipeline_layout(device, compute_pipeline.descriptor_set_layout, &compute_pipeline.layout);
	// TODO - manage descriptor set memory better.
	create_descriptor_pool(device, 256, descriptor_layout, &compute_pipeline.descriptor_pool);

	VkComputePipelineCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.layout = compute_pipeline.layout;
	create_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	create_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	create_info.stage.module = compute_shader;
	create_info.stage.pName = "main";

	VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &create_info, NULL, &compute_pipeline.handle);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Compute pipeline creation failed. (Error code: %i)", result);
	}

	vkDestroyShaderModule(device, compute_shader, NULL);

	return compute_pipeline;
}

bool destroy_compute_pipeline(compute_pipeline_t *const compute_pipeline_ptr) {

	if (compute_pipeline_ptr == NULL) {
		return false;
	}

	vkDestroyPipeline(compute_pipeline_ptr->device, compute_pipeline_ptr->handle, NULL);
	compute_pipeline_ptr->handle = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(compute_pipeline_ptr->device, compute_pipeline_ptr->layout, NULL);
	compute_pipeline_ptr->layout = VK_NULL_HANDLE;

	vkDestroyDescriptorPool(compute_pipeline_ptr->device, compute_pipeline_ptr->descriptor_pool, NULL);
	compute_pipeline_ptr->descriptor_pool = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(compute_pipeline_ptr->device, compute_pipeline_ptr->descriptor_set_layout, NULL);
	compute_pipeline_ptr->descriptor_set_layout = VK_NULL_HANDLE;

	compute_pipeline_ptr->device = VK_NULL_HANDLE;

	return true;
}
