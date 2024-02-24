#include "graphics_pipeline.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "log/logging.h"
#include "render/render_config.h"

#include "vertex_input.h"

/* -- DESCRIPTOR LAYOUT -- */

static descriptor_binding_t graphics_descriptor_bindings[2] = {
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_VERTEX_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .count = NUM_RENDER_OBJECT_SLOTS, .stages = VK_SHADER_STAGE_FRAGMENT_BIT },
};

const descriptor_layout_t graphics_descriptor_set_layout = {
	.num_bindings = 2,
	.bindings = graphics_descriptor_bindings 
};

/* -- FUNCTION DECLARATIONS -- */

static VkPipelineInputAssemblyStateCreateInfo make_input_assembly_info(void);
static VkPipelineViewportStateCreateInfo make_viewport_state_info(VkViewport *viewport_ptr, VkRect2D *scissor_ptr);
static VkPipelineRasterizationStateCreateInfo make_rasterization_info(void);
static VkPipelineMultisampleStateCreateInfo make_multisampling_info(void);
static VkPipelineColorBlendAttachmentState make_color_blend_attachment(void);
static VkPipelineColorBlendStateCreateInfo make_color_blend_info(VkPipelineColorBlendAttachmentState *attachment_ptr);

static void create_graphics_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, VkPipelineLayout *pipeline_layout_ptr);

/* -- FUNCTION DEFINITIONS -- */

static void create_render_pass(VkDevice device, VkFormat swapchain_format, VkRenderPass *render_pass_ptr) {

	log_message(VERBOSE, "Creating render pass...");

	VkAttachmentDescription color_attachment = { 0 };
	color_attachment.flags = 0;
	color_attachment.format = swapchain_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = { 0 };
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = { 0 };
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = NULL;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkSubpassDependency dependency = { 0 };
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;

	VkRenderPassCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.attachmentCount = 1;
	create_info.pAttachments = &color_attachment;
	create_info.subpassCount = 1;
	create_info.pSubpasses = &subpass;
	create_info.dependencyCount = 1;
	create_info.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(device, &create_info, NULL, render_pass_ptr);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Render pass creation failed. (Error code: %i)", result);
	}
}

graphics_pipeline_t create_graphics_pipeline(VkDevice device, swapchain_t swapchain, descriptor_layout_t descriptor_set_layout, VkShaderModule vertex_shader, VkShaderModule fragment_shader) {

	log_message(VERBOSE, "Creating graphics pipeline...");

	graphics_pipeline_t pipeline = { 0 };

	create_render_pass(device, swapchain.image_format, &pipeline.render_pass);

	VkPipelineShaderStageCreateInfo shader_stage_vertex_info = { 0 };
	shader_stage_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_vertex_info.pNext = NULL;
	shader_stage_vertex_info.flags = 0;
	shader_stage_vertex_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stage_vertex_info.module = vertex_shader;
	shader_stage_vertex_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stage_fragment_info = { 0 };
	shader_stage_fragment_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_fragment_info.pNext = NULL;
	shader_stage_fragment_info.flags = 0;
	shader_stage_fragment_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stage_fragment_info.module = fragment_shader;
	shader_stage_fragment_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[2] = { shader_stage_vertex_info, shader_stage_fragment_info };

	create_descriptor_pool(device, NUM_FRAMES_IN_FLIGHT, descriptor_set_layout, &pipeline.descriptor_pool);
	create_descriptor_set_layout(device, descriptor_set_layout, &pipeline.descriptor_set_layout);

	create_graphics_pipeline_layout(device, pipeline.descriptor_set_layout, &pipeline.layout);

	VkVertexInputBindingDescription binding_description = get_binding_description();
	VkVertexInputAttributeDescription *attribute_descriptions = get_attribute_descriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info = { 0 };
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.pNext = NULL;
	vertex_input_info.flags = 0;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = VERTEX_INPUT_NUM_ATTRIBUTES;
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;

	VkViewport viewport = make_viewport(swapchain.extent);
	VkRect2D scissor = make_scissor(swapchain.extent);

	VkPipelineInputAssemblyStateCreateInfo input_assembly = make_input_assembly_info();
	VkPipelineViewportStateCreateInfo viewport_state = make_viewport_state_info(&viewport, &scissor);
	VkPipelineRasterizationStateCreateInfo rasterizer = make_rasterization_info();
	VkPipelineMultisampleStateCreateInfo multisampling = make_multisampling_info();
	VkPipelineColorBlendAttachmentState color_blend_attachment = make_color_blend_attachment();
	VkPipelineColorBlendStateCreateInfo color_blending = make_color_blend_info(&color_blend_attachment);

	VkGraphicsPipelineCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.stageCount = 2;
	create_info.pStages = shader_stages;
	create_info.pVertexInputState = &vertex_input_info;
	create_info.pInputAssemblyState = &input_assembly;
	create_info.pViewportState = &viewport_state;
	create_info.pRasterizationState = &rasterizer;
	create_info.pMultisampleState = &multisampling;
	create_info.pDepthStencilState = NULL;
	create_info.pColorBlendState = &color_blending;
	create_info.pDynamicState = NULL;
	create_info.layout = pipeline.layout;
	create_info.renderPass = pipeline.render_pass;
	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex= -1;

	VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline.handle);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Graphics pipeline creation failed. (Error code: %i)", result);
	}

	free(attribute_descriptions);

	return pipeline;
}

void destroy_graphics_pipeline(VkDevice device, graphics_pipeline_t pipeline) {

	vkDestroyPipeline(device, pipeline.handle, NULL);
	vkDestroyPipelineLayout(device, pipeline.layout, NULL);
	vkDestroyRenderPass(device, pipeline.render_pass, NULL);

	vkDestroyDescriptorPool(device, pipeline.descriptor_pool, NULL);
	vkDestroyDescriptorSetLayout(device, pipeline.descriptor_set_layout, NULL);
}

static VkPipelineInputAssemblyStateCreateInfo make_input_assembly_info(void) {

	VkPipelineInputAssemblyStateCreateInfo input_assembly = { 0 };
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.pNext = NULL;
	input_assembly.flags = 0;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	return input_assembly;
}

static VkPipelineViewportStateCreateInfo make_viewport_state_info(VkViewport *viewport_ptr, VkRect2D *scissor_ptr) {

	VkPipelineViewportStateCreateInfo viewport_state = { 0 };
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = NULL;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = viewport_ptr;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = scissor_ptr;

	return viewport_state;
}

static VkPipelineRasterizationStateCreateInfo make_rasterization_info(void) {

	VkPipelineRasterizationStateCreateInfo rasterizer = { 0 };
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.pNext = NULL;
	rasterizer.flags = 0;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0F;
	rasterizer.depthBiasClamp = 0.0F;
	rasterizer.depthBiasSlopeFactor = 0.0F;
	rasterizer.lineWidth = 1.0F;

	return rasterizer;
}

static VkPipelineMultisampleStateCreateInfo make_multisampling_info(void) {

	VkPipelineMultisampleStateCreateInfo multisampling = { 0 };
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.pNext = NULL;
	multisampling.flags = 0;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0F;
	multisampling.pSampleMask = NULL;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	return multisampling;
}

static VkPipelineColorBlendAttachmentState make_color_blend_attachment(void) {

	VkPipelineColorBlendAttachmentState color_blend_attachment = { 0 };
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	return color_blend_attachment;
}

static VkPipelineColorBlendStateCreateInfo make_color_blend_info(VkPipelineColorBlendAttachmentState *attachment_ptr) {

	VkPipelineColorBlendStateCreateInfo color_blending = { 0 };
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.pNext = NULL;
	color_blending.flags = 0;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = attachment_ptr;
	color_blending.blendConstants[0] = 0.0F;
	color_blending.blendConstants[1] = 0.0F;
	color_blending.blendConstants[2] = 0.0F;
	color_blending.blendConstants[3] = 0.0F;

	return color_blending;
}

static void create_graphics_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, VkPipelineLayout *pipeline_layout_ptr) {

	log_message(VERBOSE, "Creating graphics pipeline layout...");

	VkPushConstantRange push_constant_ranges[2] = { { 0 } };

	// Current model slot
	push_constant_ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constant_ranges[0].offset = 0;
	push_constant_ranges[0].size = sizeof(uint32_t);

	// Current animation frame
	push_constant_ranges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	push_constant_ranges[1].offset = 0;
	push_constant_ranges[1].size = 2 * sizeof(uint32_t);

	VkPipelineLayoutCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = &descriptor_set_layout;
	create_info.pushConstantRangeCount = 2;
	create_info.pPushConstantRanges = push_constant_ranges;

	VkResult result = vkCreatePipelineLayout(device, &create_info, NULL, pipeline_layout_ptr);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Graphics pipeline layout creation failed. (Error code: %i)", result);
	}
}
