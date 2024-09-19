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
	
	// The vertex attribute index for the color value of the vertex.
	// If there is no vertex attribute for color, this will be negative.
	int32_t vertexAttributeColor;
	
	// The vertex attribute index for the texture coordinates of the vertex.
	// If there is no vertex attribute for texture coordinates, this will be negative.
	// Also controls whether a texture is written to the descriptor set.
	int32_t vertexAttributeTextureCoords;
	
	VkPipeline vkPipeline;
	VkPipelineLayout vkPipelineLayout;
	
	VkDescriptorPool vkDescriptorPool;
	VkDescriptorSetLayout vkDescriptorSetLayout;
	
	VkDevice vkDevice;
	
} GraphicsPipeline;

typedef enum VertexAttributeTypes {
	VERTEX_ATTRIBUTE_POSITION = 0x00000001,
	VERTEX_ATTRIBUTE_TEXTURE_COORDINATES = 0x00000002,
	VERTEX_ATTRIBUTE_COLOR = 0x00000004
} VertexAttributeTypes;

typedef struct GraphicsPipelineCreateInfo {
	
	VkDevice vkDevice;
	
	Swapchain swapchain;
	
	VkPrimitiveTopology topology;
	VkPolygonMode polygonMode;
	
	// Array of vertex attributes, described by their element counts.
	uint32_t vertexAttributeCount;
	uint32_t *pVertexAttributeSizes;
	
	// Bitflags that control which vertex attributes the pipeline will have.
	uint32_t vertexAttributeFlags;
	
	// TODO - change to just array of descriptor bindings.
	DescriptorSetLayout descriptorSetLayout;
	
	// Array of shader modules to attach to the pipeline.
	uint32_t shaderModuleCount;
	ShaderModule *pShaderModules;
	
	// Array of push constant ranges to include in the pipeline layout.
	uint32_t pushConstantRangeCount;
	PushConstantRange *pPushConstantRanges;
	
} GraphicsPipelineCreateInfo;

GraphicsPipeline createGraphicsPipeline(const GraphicsPipelineCreateInfo createInfo);

void deleteGraphicsPipeline(GraphicsPipeline *const pPipeline);

#endif	// GRAPHICS_PIPELINE_H
