#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "descriptor.h"
#include "Pipeline.h"
#include "Shader.h"
#include "Swapchain.h"

typedef struct GraphicsPipelineCreateInfo {
	
	VkDevice vkDevice;
	
	Swapchain swapchain;
	
	VkPrimitiveTopology topology;
	VkPolygonMode polygonMode;
	
	// TODO - change to just array of descriptor bindings.
	DescriptorSetLayout descriptorSetLayout;
	
	uint32_t shaderModuleCount;
	ShaderModule *pShaderModules;
	
	uint32_t pushConstantRangeCount;
	PushConstantRange *pPushConstantRanges;
	
} GraphicsPipelineCreateInfo;
		
Pipeline createGraphicsPipeline2(const GraphicsPipelineCreateInfo createInfo);

#endif	// GRAPHICS_PIPELINE_H
