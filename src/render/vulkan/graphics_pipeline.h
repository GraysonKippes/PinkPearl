#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "descriptor.h"
#include "swapchain.h"

typedef struct graphics_pipeline_t {

	VkPipeline m_handle;
	VkPipelineLayout m_layout;
	VkRenderPass m_render_pass;

} graphics_pipeline_t;

graphics_pipeline_t create_graphics_pipeline(VkDevice device, swapchain_t swapchain, VkDescriptorSetLayout descriptor_set_layout, VkShaderModule vertex_shader, VkShaderModule fragment_shader);

void destroy_graphics_pipeline(VkDevice device, graphics_pipeline_t pipeline);

extern const descriptor_layout_t graphics_descriptor_layout;

#endif	// GRAPHICS_PIPELINE_H
