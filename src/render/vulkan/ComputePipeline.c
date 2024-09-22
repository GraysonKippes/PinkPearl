#include "ComputePipeline.h"

#include <stdlib.h>

#include "Descriptor.h"
#include "shader.h"

#include "log/Logger.h"

Pipeline createComputePipeline(const ComputePipelineCreateInfo createInfo) {
	
	Pipeline computePipeline = { .type = PIPELINE_TYPE_COMPUTE };
	
	ShaderModule shaderModule = createShaderModule(createInfo.vkDevice, SHADER_STAGE_COMPUTE, createInfo.shaderFilename.pBuffer);
	computePipeline.vkPipelineLayout = createPipelineLayout(createInfo.vkDevice, 1, &globalDescriptorSetLayout, createInfo.pushConstantRangeCount, createInfo.pPushConstantRanges);

	const VkComputePipelineCreateInfo vkComputePipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.layout = computePipeline.vkPipelineLayout,
		.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.stage.module = shaderModule.vkShaderModule,
		.stage.pName = "main"
	};

	const VkResult result = vkCreateComputePipelines(createInfo.vkDevice, VK_NULL_HANDLE, 1, &vkComputePipelineCreateInfo, nullptr, &computePipeline.vkPipeline);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_FATAL, "Compute pipeline creation failed (error code: %i).", result);
	}
	computePipeline.vkDevice = createInfo.vkDevice;

	destroyShaderModule(&shaderModule);
	return computePipeline;
}
