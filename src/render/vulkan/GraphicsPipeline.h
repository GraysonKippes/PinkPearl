#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "descriptor.h"
#include "Pipeline.h"
#include "Shader.h"
#include "Swapchain.h"

typedef struct GraphicsPipeline {
	
	// The element offset of the position vertex attribute.
	// Used for building meshes to be rendered with this pipeline.
	// This value is negative if this pipeline has no vertex attribute for position.
	int32_t vertexAttributePositionOffset;
	
	// The element offset of the texture coordinates vertex attribute.
	// Used for building meshes to be rendered with this pipeline.
	// This value is negative if this pipeline has no vertex attribute for texture coordinates.
	int32_t vertexAttributeTextureCoordinatesOffset;
	
	// The element offset of the color vertex attribute.
	// Used for building meshes to be rendered with this pipeline.
	// This value is negative if this pipeline has no vertex attribute for color.
	int32_t vertexAttributeColorOffset;
	
	// The number of elements in a single vertex.
	// Used for building meshes to be rendered with this pipeline.
	int32_t vertexInputElementStride;
	
	VkPipeline vkPipeline;
	VkPipelineLayout vkPipelineLayout;
	
	//VkDescriptorPool vkDescriptorPool;
	//VkDescriptorSetLayout vkDescriptorSetLayout;
	
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
	
	// Bitflags that control which vertex attributes the pipeline will have.
	uint32_t vertexAttributeFlags;
	
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
