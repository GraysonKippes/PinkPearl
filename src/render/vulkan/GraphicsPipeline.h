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
	
	// The number of elements in a single vertex, equal to the sum of all the attribute sizes.
	uint32_t vertexElementStride;
	
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
	
	// Array of vertex attributes, described by their element counts.
	uint32_t vertexAttributeCount;
	uint32_t *pVertexAttributeSizes;
	
	// TODO - change to just array of descriptor bindings.
	DescriptorSetLayout descriptorSetLayout;
	
	// Array of shader modules to attach to the pipeline.
	uint32_t shaderModuleCount;
	ShaderModule *pShaderModules;
	
	// Array of push constant ranges to include in the pipeline layout.
	uint32_t pushConstantRangeCount;
	PushConstantRange *pPushConstantRanges;
	
} GraphicsPipelineCreateInfo;

GraphicsPipeline createGraphicsPipeline2(const GraphicsPipelineCreateInfo createInfo);

void deleteGraphicsPipeline(GraphicsPipeline *const pPipeline);

#endif	// GRAPHICS_PIPELINE_H
