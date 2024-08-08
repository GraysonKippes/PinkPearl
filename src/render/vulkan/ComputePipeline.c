#include "ComputePipeline.h"

#include <stdlib.h>

#include "shader.h"

#include "log/Logger.h"

void create_pipeline_layout(VkDevice vkDevice, VkDescriptorSetLayout vkDescriptorSetLayout, VkPipelineLayout *pPipelineLayout) {

	const VkPipelineLayoutCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &vkDescriptorSetLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	vkCreatePipelineLayout(vkDevice, &createInfo, nullptr, pPipelineLayout);
}

Pipeline createComputePipeline(const VkDevice vkDevice, const DescriptorSetLayout descriptorSetLayout, const char *const pComputeShaderFilename) {
	
	Pipeline computePipeline = { };
	computePipeline.type = PIPELINE_TYPE_COMPUTE;
	
	ShaderModule compute_shader_module = create_shader_module(vkDevice, pComputeShaderFilename);

	create_descriptor_set_layout(vkDevice, descriptorSetLayout, &computePipeline.vkDescriptorSetLayout);
	create_pipeline_layout(vkDevice, computePipeline.vkDescriptorSetLayout, &computePipeline.vkPipelineLayout);
	// TODO - manage descriptor set memory better.
	create_descriptor_pool(vkDevice, 256, descriptorSetLayout, &computePipeline.vkDescriptorPool);

	const VkComputePipelineCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.layout = computePipeline.vkPipelineLayout,
		.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.stage.module = compute_shader_module.module_handle,
		.stage.pName = "main"
	};

	const VkResult result = vkCreateComputePipelines(vkDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &computePipeline.vkPipeline);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_FATAL, "Compute pipeline creation failed (error code: %i).", result);
	}
	computePipeline.vkDevice = vkDevice;

	destroy_shader_module(&compute_shader_module);
	return computePipeline;
}
