#include "graphics_pipeline.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "game/area/room.h"
#include "log/logging.h"
#include "render/render_config.h"

#include "vertex_input.h"
#include "vulkan_manager.h"

/* -- DESCRIPTOR LAYOUT -- */

static DescriptorBinding graphics_descriptor_bindings[4] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },	// Draw data
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_VERTEX_BIT },	// Matrix buffer
	{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .count = VK_CONF_MAX_NUM_QUADS, .stages = VK_SHADER_STAGE_FRAGMENT_BIT },	// Texture array
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_FRAGMENT_BIT }	// Lighting
};

const DescriptorSetLayout graphics_descriptor_set_layout = {
	.num_bindings = 4,
	.bindings = graphics_descriptor_bindings 
};

/* -- FUNCTION DECLARATIONS -- */

static VkPipelineInputAssemblyStateCreateInfo make_input_assembly_info(void);
static VkPipelineViewportStateCreateInfo make_viewport_state_info(VkViewport *viewport_ptr, VkRect2D *scissor_ptr);
static VkPipelineRasterizationStateCreateInfo make_rasterization_info(void);
static VkPipelineMultisampleStateCreateInfo make_multisampling_info(void);
static VkPipelineColorBlendAttachmentState make_color_blend_attachment(void);
static VkPipelineColorBlendStateCreateInfo make_color_blend_info(VkPipelineColorBlendAttachmentState attachmentState[static 1]);
static void create_graphics_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, VkPipelineLayout *pipeline_layout_ptr);

/* -- FUNCTION DEFINITIONS -- */

static VkRenderPass createRenderPass(VkDevice device, VkFormat swapchain_format) {
	logMsg(LOG_LEVEL_VERBOSE, "Creating render pass...");

	const VkAttachmentDescription2 attachmentDescriptions[2] = {
		{	// Color attachment description
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
			.pNext = nullptr,
			.flags = 0,
			.format = swapchain_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		},
		{	// Depth attachment description
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
			.pNext = nullptr,
			.flags = 0,
			.format = VK_FORMAT_D32_SFLOAT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}
	};

	const VkAttachmentReference2 attachmentReferences[2] = {
		{	// Color attachment reference
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
			.pNext = nullptr,
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
		},
		{	// Depth attachment reference
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
			.pNext = nullptr,
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT
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

	VkRenderPassCreateInfo2 renderPassCreateInfo = {
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
	const VkResult result = vkCreateRenderPass2(device, &renderPassCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS) {
		logMsgF(LOG_LEVEL_FATAL, "Render pass creation failed. (Error code: %i)", result);
	}
	return renderPass;
}

GraphicsPipeline create_graphics_pipeline(VkDevice device, Swapchain swapchain, DescriptorSetLayout descriptor_set_layout, VkShaderModule vertex_shader, VkShaderModule fragment_shader) {

	logMsg(LOG_LEVEL_VERBOSE, "Creating graphics pipeline...");

	GraphicsPipeline pipeline = { };
	pipeline.render_pass = createRenderPass(device, swapchain.image_format);

	VkPipelineShaderStageCreateInfo shader_stage_vertex_info = { 0 };
	shader_stage_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_vertex_info.pNext = nullptr;
	shader_stage_vertex_info.flags = 0;
	shader_stage_vertex_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stage_vertex_info.module = vertex_shader;
	shader_stage_vertex_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stage_fragment_info = { 0 };
	shader_stage_fragment_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_fragment_info.pNext = nullptr;
	shader_stage_fragment_info.flags = 0;
	shader_stage_fragment_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stage_fragment_info.module = fragment_shader;
	shader_stage_fragment_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[2] = { shader_stage_vertex_info, shader_stage_fragment_info };

	create_descriptor_pool(device, NUM_FRAMES_IN_FLIGHT, descriptor_set_layout, &pipeline.descriptor_pool);
	create_descriptor_set_layout(device, descriptor_set_layout, &pipeline.descriptor_set_layout);

	create_graphics_pipeline_layout(device, pipeline.descriptor_set_layout, &pipeline.layout);

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

	VkPipelineInputAssemblyStateCreateInfo input_assembly = make_input_assembly_info();
	VkPipelineViewportStateCreateInfo viewport_state = make_viewport_state_info(&viewport, &scissor);
	VkPipelineRasterizationStateCreateInfo rasterizer = make_rasterization_info();
	VkPipelineMultisampleStateCreateInfo multisampling = make_multisampling_info();
	VkPipelineColorBlendAttachmentState color_blend_attachment = make_color_blend_attachment();
	VkPipelineColorBlendStateCreateInfo color_blending = make_color_blend_info(&color_blend_attachment);
	
	const VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = { },
		.back = { },
		.minDepthBounds = 0.0F,
		.maxDepthBounds = 1.0F
	};

	const VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = 2,
		.pStages = shader_stages,
		.pVertexInputState = &vertex_input_info,
		.pInputAssemblyState = &input_assembly,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = nullptr,
		.pColorBlendState = &color_blending,
		.pDynamicState = nullptr,
		.layout = pipeline.layout,
		.renderPass = pipeline.render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex= -1
	};

	const VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline.handle);
	if (result != VK_SUCCESS) {
		logMsgF(LOG_LEVEL_FATAL, "Graphics pipeline creation failed. (Error code: %i)", result);
	}

	return pipeline;
}

void destroy_graphics_pipeline(VkDevice device, GraphicsPipeline pipeline) {

	vkDestroyPipeline(device, pipeline.handle, nullptr);
	vkDestroyPipelineLayout(device, pipeline.layout, nullptr);
	vkDestroyRenderPass(device, pipeline.render_pass, nullptr);

	vkDestroyDescriptorPool(device, pipeline.descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(device, pipeline.descriptor_set_layout, nullptr);
}

static VkPipelineInputAssemblyStateCreateInfo make_input_assembly_info(void) {

	VkPipelineInputAssemblyStateCreateInfo input_assembly = { 0 };
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.pNext = nullptr;
	input_assembly.flags = 0;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	return input_assembly;
}

static VkPipelineViewportStateCreateInfo make_viewport_state_info(VkViewport *viewport_ptr, VkRect2D *scissor_ptr) {

	VkPipelineViewportStateCreateInfo viewport_state = { 0 };
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = nullptr;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = viewport_ptr;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = scissor_ptr;

	return viewport_state;
}

static VkPipelineRasterizationStateCreateInfo make_rasterization_info(void) {
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

static VkPipelineMultisampleStateCreateInfo make_multisampling_info(void) {

	VkPipelineMultisampleStateCreateInfo multisampling = { 0 };
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.pNext = nullptr;
	multisampling.flags = 0;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0F;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	return multisampling;
}

static VkPipelineColorBlendAttachmentState make_color_blend_attachment(void) {
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

static VkPipelineColorBlendStateCreateInfo make_color_blend_info(VkPipelineColorBlendAttachmentState attachmentState[static 1]) {
	return (VkPipelineColorBlendStateCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = attachmentState,
		.blendConstants = { 0.0F, 0.0F, 0.0F, 0.0F }
	};
	
}

static void create_graphics_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, VkPipelineLayout *pipeline_layout_ptr) {

	logMsg(LOG_LEVEL_VERBOSE, "Creating graphics pipeline layout...");

	VkPipelineLayoutCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = &descriptor_set_layout;
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(device, &create_info, nullptr, pipeline_layout_ptr);
	if (result != VK_SUCCESS) {
		logMsgF(LOG_LEVEL_FATAL, "Graphics pipeline layout creation failed. (Error code: %i)", result);
	}
}
