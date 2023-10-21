#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "swapchain.h"

typedef struct graphics_pipeline_t {

	VkPipeline m_handle;
	VkPipelineLayout m_layout;
	VkRenderPass m_render_pass;

} graphics_pipeline_t;

graphics_pipeline_t create_graphics_pipeline(VkDevice logical_device, swapchain_t swapchain, VkDescriptorSetLayout descriptor_set_layout, VkShaderModule vertex_shader, VkShaderModule fragment_shader);

#endif	// GRAPHICS_PIPELINE_H
