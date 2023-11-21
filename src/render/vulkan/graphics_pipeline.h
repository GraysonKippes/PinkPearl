#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "descriptor.h"
#include "swapchain.h"

extern const descriptor_layout_t graphics_descriptor_set_layout;

typedef struct graphics_pipeline_t {

	VkPipeline handle;
	VkPipelineLayout layout;
	VkRenderPass render_pass;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;

} graphics_pipeline_t;

graphics_pipeline_t create_graphics_pipeline(VkDevice device, swapchain_t swapchain, descriptor_layout_t descriptor_set_layout, VkShaderModule vertex_shader, VkShaderModule fragment_shader);

void destroy_graphics_pipeline(VkDevice device, graphics_pipeline_t pipeline);

#endif	// GRAPHICS_PIPELINE_H
