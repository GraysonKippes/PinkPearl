#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "descriptor.h"
#include "Pipeline.h"
#include "Shader.h"
#include "Swapchain.h"

typedef struct GraphicsPipeline {
	
	// Array of vertex attributes, described by their element counts.
	uint32_t vertexAttributeCount;
	uint32_t *pVertexAttributeSizes;
	
	VkPipeline vkPipeline;
	VkPipelineLayout vkPipelineLayout;
	
	VkDescriptorPool vkDescriptorPool;
	VkDescriptorSetLayout vkDescriptorSetLayout;
	
	VkDevice vkDevice;
	
} GraphicsPipeline;

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
		
Pipeline createGraphicsPipeline(const GraphicsPipelineCreateInfo createInfo);

#endif	// GRAPHICS_PIPELINE_H
