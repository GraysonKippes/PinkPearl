#include "Pipeline.h"

#include "log/Logger.h"

#include "memory.h"

bool validatePipeline(const Pipeline pipeline) {
	return pipeline.vkPipeline != VK_NULL_HANDLE
		&& pipeline.vkPipelineLayout != VK_NULL_HANDLE
		&& pipeline.vkDevice != VK_NULL_HANDLE;
}

bool deletePipeline(Pipeline *const pPipeline) {
	if (!pPipeline) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting pipeline: pointer to pipeline object is null.");
		return false;
	} else if (!validatePipeline(*pPipeline)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting pipeline: pipeline object is invalid.");
		return false;
	}
	
	// Destroy the Vulkan objects associated with the pipeline struct.
	vkDestroyPipeline(pPipeline->vkDevice, pPipeline->vkPipeline, nullptr);
	vkDestroyPipelineLayout(pPipeline->vkDevice, pPipeline->vkPipelineLayout, nullptr);
	
	// Nullify the handles inside the struct.
	pPipeline->vkPipeline = VK_NULL_HANDLE;
	pPipeline->vkPipelineLayout = VK_NULL_HANDLE;
	pPipeline->vkDevice = VK_NULL_HANDLE;

	return true;
}

VkPipelineLayout createPipelineLayout(const VkDevice vkDevice, 
		const uint32_t vkDescriptorSetLayoutCount, const VkDescriptorSetLayout vkDescriptorSetLayouts[static const vkDescriptorSetLayoutCount],
		const uint32_t pushConstantRangeCount, const PushConstantRange pushConstantRanges[static const pushConstantRangeCount]) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating pipeline layout...");

	uint32_t offsetAccummulator = 0;
	VkPushConstantRange vkPushConstantRanges[pushConstantRangeCount];
	for (uint32_t i = 0; i < pushConstantRangeCount; ++i) {
		vkPushConstantRanges[i] = (VkPushConstantRange){
			.stageFlags = pushConstantRanges[i].shaderStageFlags,
			.offset = offsetAccummulator,
			.size = pushConstantRanges[i].size
		};
		offsetAccummulator += alignOffset(pushConstantRanges[i].size, 4);
	}

	const VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = vkDescriptorSetLayoutCount,
		.pSetLayouts = vkDescriptorSetLayouts,
		.pushConstantRangeCount = pushConstantRangeCount,
		.pPushConstantRanges = vkPushConstantRanges
	};

	VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VkResult result = vkCreatePipelineLayout(vkDevice, &vkPipelineLayoutCreateInfo, nullptr, &vkPipelineLayout);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Graphics pipeline layout creation failed (error code: %i).", result);
	}
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Created pipeline layout.");
	return vkPipelineLayout;
}