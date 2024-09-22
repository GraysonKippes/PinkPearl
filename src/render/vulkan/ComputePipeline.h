#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "util/String.h"

#include "descriptor.h"
#include "Pipeline.h"

typedef struct ComputePipelineCreateInfo {
	
	VkDevice vkDevice;
	
	String shaderFilename;
	
	// Array of push constant ranges to include in the pipeline layout.
	uint32_t pushConstantRangeCount;
	PushConstantRange *pPushConstantRanges;
	
} ComputePipelineCreateInfo;

Pipeline createComputePipeline(const ComputePipelineCreateInfo createInfo);

#endif	// COMPUTE_PIPELINE_H
