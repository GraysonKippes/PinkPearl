#include "compute_pipeline.h"

#include <stdlib.h>

#include "shader.h"

#include "log/logging.h"



static descriptor_binding_t compute_matrices_bindings[2] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT }
};

const descriptor_layout_t compute_matrices_layout = {
	.num_bindings = 2,
	.bindings = compute_matrices_bindings
};



static descriptor_binding_t compute_room_texture_bindings[3] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT }
};

const descriptor_layout_t compute_room_texture_layout = {
	.num_bindings = 3,
	.bindings = compute_room_texture_bindings
};



static descriptor_binding_t compute_textures_bindings[3] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT }
};

const descriptor_layout_t compute_textures_layout = {
	.num_bindings = 3,
	.bindings = compute_textures_bindings
};



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

compute_pipeline_t create_compute_pipeline(VkDevice device, descriptor_layout_t descriptor_layout, const char *compute_shader_name) {
	
	VkShaderModule compute_shader;

	create_shader_module(device, compute_shader_name, &compute_shader);

	compute_pipeline_t compute_pipeline = { 0 };

	create_descriptor_set_layout(device, descriptor_layout, &compute_pipeline.descriptor_set_layout);
	create_pipeline_layout(device, compute_pipeline.descriptor_set_layout, &compute_pipeline.layout);
	create_descriptor_pool(device, 1, descriptor_layout, &compute_pipeline.descriptor_pool);

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

void destroy_compute_pipeline(VkDevice device, compute_pipeline_t compute_pipeline) {
	vkDestroyPipeline(device, compute_pipeline.handle, NULL);
	vkDestroyPipelineLayout(device, compute_pipeline.layout, NULL);
	vkDestroyDescriptorSetLayout(device, compute_pipeline.descriptor_set_layout, NULL);
	vkDestroyDescriptorPool(device, compute_pipeline.descriptor_pool, NULL);
}
