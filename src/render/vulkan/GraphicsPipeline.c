#include "GraphicsPipeline.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "game/area/room.h"
#include "log/Logger.h"
#include "render/render_config.h"

#include "vertex_input.h"
#include "vulkan_manager.h"

/* -- FUNCTION DECLARATIONS -- */

static VkPipelineInputAssemblyStateCreateInfo makePipelineInputAssemblyStateCreateInfo(void);

static VkPipelineViewportStateCreateInfo makePipelineViewportStateCreateInfo(const VkViewport *const pViewport, const VkRect2D *const pScissor);

static VkPipelineRasterizationStateCreateInfo makePipelineRasterizationStateCreateInfo(void);

static VkPipelineMultisampleStateCreateInfo makePipelineMultisampleStateCreateInfo(void);

static VkPipelineColorBlendAttachmentState makePipelineColorBlendAttachmentState(void);

static VkPipelineColorBlendStateCreateInfo makePipelineColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState *const pAttachmentState);

/* -- FUNCTION DEFINITIONS -- */

VkRenderPass createRenderPass(const VkDevice vkDevice, const VkFormat swapchainFormat) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating render pass...");

	const VkAttachmentDescription2 attachmentDescriptions[1] = {
		{	// Color attachment description
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
			.pNext = nullptr,
			.flags = 0,
			.format = swapchainFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		}
	};

	const VkAttachmentReference2 attachmentReferences[1] = {
		{	// Color attachment reference
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
			.pNext = nullptr,
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
		}
	};

	const VkSubpassDescription2 subpassDescription = {
		.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.viewMask = 0,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachmentReferences[0],
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = nullptr,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	const VkSubpassDependency2 subpassDependency = {
		.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
		.pNext = nullptr,
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_NONE,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0,
		.viewOffset = 0
	};

	const VkRenderPassCreateInfo2 renderPassCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = attachmentDescriptions,
		.subpassCount = 1,
		.pSubpasses = &subpassDescription,
		.dependencyCount = 1,
		.pDependencies = &subpassDependency,
		.correlatedViewMaskCount = 0,
		.pCorrelatedViewMasks = nullptr
	};

	VkRenderPass renderPass = VK_NULL_HANDLE;
	const VkResult result = vkCreateRenderPass2(vkDevice, &renderPassCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_FATAL, "Render pass creation failed. (Error code: %i)", result);
	}
	
	return renderPass;
}

Pipeline createGraphicsPipeline(const VkDevice vkDevice, const Swapchain swapchain, const VkRenderPass renderPass, const DescriptorSetLayout descriptorSetLayout, 
		const uint32_t shaderModuleCount, const ShaderModule shaderModules[static const shaderModuleCount]) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating graphics pipeline...");

	Pipeline pipeline = { .type = PIPELINE_TYPE_GRAPHICS };

	VkPipelineShaderStageCreateInfo shaderStateCreateInfos[shaderModuleCount];
	for (uint32_t i = 0; i < shaderModuleCount; ++i) {
		shaderStateCreateInfos[i] = makeShaderStageCreateInfo(shaderModules[i]);
	}

	create_descriptor_pool(vkDevice, NUM_FRAMES_IN_FLIGHT, descriptorSetLayout, &pipeline.vkDescriptorPool);
	create_descriptor_set_layout(vkDevice, descriptorSetLayout, &pipeline.vkDescriptorSetLayout);

	pipeline.vkPipelineLayout = createPipelineLayout(vkDevice, pipeline.vkDescriptorSetLayout);

	VkVertexInputBindingDescription binding_description = get_binding_description();

	VkVertexInputAttributeDescription attribute_descriptions[VERTEX_INPUT_NUM_ATTRIBUTES] = { { 0 } };
	get_attribute_descriptions(attribute_descriptions);

	const VkPipelineVertexInputStateCreateInfo vertex_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding_description,
		.vertexAttributeDescriptionCount = vertex_input_num_attributes,
		.pVertexAttributeDescriptions = attribute_descriptions
	};

	VkViewport viewport = make_viewport(swapchain.extent);
	VkRect2D scissor = make_scissor(swapchain.extent);

	VkPipelineInputAssemblyStateCreateInfo input_assembly = makePipelineInputAssemblyStateCreateInfo();
	VkPipelineViewportStateCreateInfo viewport_state = makePipelineViewportStateCreateInfo(&viewport, &scissor);
	VkPipelineRasterizationStateCreateInfo rasterizer = makePipelineRasterizationStateCreateInfo();
	VkPipelineMultisampleStateCreateInfo multisampling = makePipelineMultisampleStateCreateInfo();
	VkPipelineColorBlendAttachmentState color_blend_attachment = makePipelineColorBlendAttachmentState();
	VkPipelineColorBlendStateCreateInfo color_blending = makePipelineColorBlendStateCreateInfo(&color_blend_attachment);

	const VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = shaderModuleCount,
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
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex= -1
	};

	const VkResult result = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline.vkPipeline);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_FATAL, "Graphics pipeline creation failed (error code: %i).", result);
	}

	return pipeline;
}

static VkPipelineInputAssemblyStateCreateInfo makePipelineInputAssemblyStateCreateInfo(void) {
	return (VkPipelineInputAssemblyStateCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
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

static VkPipelineRasterizationStateCreateInfo makePipelineRasterizationStateCreateInfo(void) {
	return (VkPipelineRasterizationStateCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
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
