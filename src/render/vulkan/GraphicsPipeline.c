#include "GraphicsPipeline.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "game/area/room.h"
#include "log/Logger.h"
#include "render/render_config.h"

#include "VulkanManager.h"

/* -- FUNCTION DECLARATIONS -- */

static VkPipelineInputAssemblyStateCreateInfo makePipelineInputAssemblyStateCreateInfo(const VkPrimitiveTopology topology);

static VkPipelineViewportStateCreateInfo makePipelineViewportStateCreateInfo(const VkViewport *const pViewport, const VkRect2D *const pScissor);

static VkPipelineRasterizationStateCreateInfo makePipelineRasterizationStateCreateInfo(const VkPolygonMode polygonMode);

static VkPipelineMultisampleStateCreateInfo makePipelineMultisampleStateCreateInfo(void);

static VkPipelineColorBlendAttachmentState makePipelineColorBlendAttachmentState(void);

static VkPipelineColorBlendStateCreateInfo makePipelineColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState *const pAttachmentState);

/* -- FUNCTION DEFINITIONS -- */

GraphicsPipeline createGraphicsPipeline(const GraphicsPipelineCreateInfo createInfo) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating graphics pipeline...");

	GraphicsPipeline pipeline = { 
		.vertexAttributePositionOffset = -1,
		.vertexAttributeTextureCoordinatesOffset = -1,
		.vertexAttributeColorOffset = -1
	};

	VkPipelineShaderStageCreateInfo shaderStateCreateInfos[createInfo.shaderModuleCount];
	for (uint32_t i = 0; i < createInfo.shaderModuleCount; ++i) {
		shaderStateCreateInfos[i] = makeShaderStageCreateInfo(createInfo.pShaderModules[i]);
	}

	create_descriptor_pool(createInfo.vkDevice, NUM_FRAMES_IN_FLIGHT, createInfo.descriptorSetLayout, &pipeline.vkDescriptorPool);
	create_descriptor_set_layout(createInfo.vkDevice, createInfo.descriptorSetLayout, &pipeline.vkDescriptorSetLayout);

	pipeline.vkPipelineLayout = createPipelineLayout2(createInfo.vkDevice, 1, &pipeline.vkDescriptorSetLayout, createInfo.pushConstantRangeCount, createInfo.pPushConstantRanges);

	// Find the number of vertex attributes by counting the number of bits in the vertex attribute type bitflags.
	uint32_t vertexAttributeCount = 0;
	for (uint32_t i = 0; i < sizeof(createInfo.vertexAttributeFlags) * 8; ++i) {
		vertexAttributeCount += (createInfo.vertexAttributeFlags >> i) & 1;
	}
	VkVertexInputAttributeDescription attributeDescriptions[vertexAttributeCount] = { };
	VkVertexInputBindingDescription bindingDescription = { };

	{	// Generate the vertex attribute descriptions and element offsets as well as the vertex binding description.
		uint32_t counter = 0;	// The number of vertex attributes as they are added.
		uint32_t offset = 0;	// The element offset of each attribute.
		
		if (createInfo.vertexAttributeFlags & VERTEX_ATTRIBUTE_POSITION && counter < vertexAttributeCount) {
			attributeDescriptions[counter] = (VkVertexInputAttributeDescription){
				.binding = 0,
				.location = counter,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offset * sizeof(float)
			};
			pipeline.vertexAttributePositionOffset = offset;
			offset += 3;
			counter += 1;
		}
		
		if (createInfo.vertexAttributeFlags & VERTEX_ATTRIBUTE_TEXTURE_COORDINATES && counter < vertexAttributeCount) {
			attributeDescriptions[counter] = (VkVertexInputAttributeDescription){
				.binding = 0,
				.location = counter,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offset * sizeof(float)
			};
			pipeline.vertexAttributeTextureCoordinatesOffset = offset;
			offset += 2;
			counter += 1;
		}
		
		if (createInfo.vertexAttributeFlags & VERTEX_ATTRIBUTE_COLOR && counter < vertexAttributeCount) {
			attributeDescriptions[counter] = (VkVertexInputAttributeDescription){
				.binding = 0,
				.location = counter,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offset * sizeof(float)
			};
			pipeline.vertexAttributeColorOffset = offset;
			offset += 3;
			counter += 1;
		}
		
		bindingDescription = (VkVertexInputBindingDescription){
			.binding = 0,
			.stride = offset * sizeof(float),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};
		pipeline.vertexInputElementStride = offset;
	}
	
	const VkPipelineVertexInputStateCreateInfo vertex_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = vertexAttributeCount,
		.pVertexAttributeDescriptions = attributeDescriptions
	};

	VkViewport viewport = makeViewport(createInfo.swapchain.imageExtent);
	VkRect2D scissor = makeScissor(createInfo.swapchain.imageExtent);

	VkPipelineInputAssemblyStateCreateInfo input_assembly = makePipelineInputAssemblyStateCreateInfo(createInfo.topology);
	VkPipelineViewportStateCreateInfo viewport_state = makePipelineViewportStateCreateInfo(&viewport, &scissor);
	VkPipelineRasterizationStateCreateInfo rasterizer = makePipelineRasterizationStateCreateInfo(createInfo.polygonMode);
	VkPipelineMultisampleStateCreateInfo multisampling = makePipelineMultisampleStateCreateInfo();
	VkPipelineColorBlendAttachmentState color_blend_attachment = makePipelineColorBlendAttachmentState();
	VkPipelineColorBlendStateCreateInfo color_blending = makePipelineColorBlendStateCreateInfo(&color_blend_attachment);

	const VkPipelineRenderingCreateInfo renderingInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.pNext = nullptr,
		.viewMask = 0,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &createInfo.swapchain.imageFormat,
		.depthAttachmentFormat = VK_FORMAT_UNDEFINED,
		.stencilAttachmentFormat = VK_FORMAT_UNDEFINED
	};

	const VkGraphicsPipelineCreateInfo vkGraphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &renderingInfo,
		.flags = 0,
		.stageCount = createInfo.shaderModuleCount,
		.pStages = shaderStateCreateInfos,
		.pVertexInputState = &vertex_input_info,
		.pInputAssemblyState = &input_assembly,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = nullptr,
		.pColorBlendState = &color_blending,
		.pDynamicState = nullptr,
		.layout = pipeline.vkPipelineLayout,
		.renderPass = VK_NULL_HANDLE,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex= -1
	};

	const VkResult result = vkCreateGraphicsPipelines(createInfo.vkDevice, VK_NULL_HANDLE, 1, &vkGraphicsPipelineCreateInfo, nullptr, &pipeline.vkPipeline);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Graphics pipeline creation failed (error code: %i).", result);
	}
	pipeline.vkDevice = createInfo.vkDevice;

	return pipeline;
}

void deleteGraphicsPipeline(GraphicsPipeline *const pPipeline) {
	if (!pPipeline) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting pipeline: pointer to pipeline object is null.");
		return;
	}
	
	// Destroy the Vulkan objects associated with the pipeline struct.
	vkDestroyPipeline(pPipeline->vkDevice, pPipeline->vkPipeline, nullptr);
	vkDestroyPipelineLayout(pPipeline->vkDevice, pPipeline->vkPipelineLayout, nullptr);
	vkDestroyDescriptorPool(pPipeline->vkDevice, pPipeline->vkDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pPipeline->vkDevice, pPipeline->vkDescriptorSetLayout, nullptr);
	
	// Nullify the objects inside the struct.
	pPipeline->vkPipeline = VK_NULL_HANDLE;
	pPipeline->vkPipelineLayout = VK_NULL_HANDLE;
	pPipeline->vkDescriptorPool = VK_NULL_HANDLE;
	pPipeline->vkDescriptorSetLayout = VK_NULL_HANDLE;
	pPipeline->vkDevice = VK_NULL_HANDLE;
}

static VkPipelineInputAssemblyStateCreateInfo makePipelineInputAssemblyStateCreateInfo(const VkPrimitiveTopology topology) {
	return (VkPipelineInputAssemblyStateCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = topology,
		.primitiveRestartEnable = VK_FALSE
	};
}

static VkPipelineViewportStateCreateInfo makePipelineViewportStateCreateInfo(const VkViewport *const pViewport, const VkRect2D *const pScissor) {
	return (VkPipelineViewportStateCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = pViewport,
		.scissorCount = 1,
		.pScissors = pScissor	
	};
}

static VkPipelineRasterizationStateCreateInfo makePipelineRasterizationStateCreateInfo(const VkPolygonMode polygonMode) {
	return (VkPipelineRasterizationStateCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = polygonMode,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0F,
		.depthBiasClamp = 0.0F,
		.depthBiasSlopeFactor = 0.0F,
		.lineWidth = 1.0F
	};
}

static VkPipelineMultisampleStateCreateInfo makePipelineMultisampleStateCreateInfo(void) {
	return (VkPipelineMultisampleStateCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0F,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};
}

static VkPipelineColorBlendAttachmentState makePipelineColorBlendAttachmentState(void) {
	return (VkPipelineColorBlendAttachmentState){
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD
	};
}

static VkPipelineColorBlendStateCreateInfo makePipelineColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState *const pAttachmentState) {
	return (VkPipelineColorBlendStateCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = pAttachmentState,
		.blendConstants = { 0.0F, 0.0F, 0.0F, 0.0F }
	};
}