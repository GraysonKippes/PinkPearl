#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

typedef struct compute_shader_t {

	VkShaderModule m_module;
	VkDescriptorSetLayout m_descriptor_set_layout;

} compute_shader_t;

typedef struct compute_pipeline_t {

	VkPipeline m_handle;
	VkPipelineLayout m_layout;
	
} compute_pipeline_t;

void create_compute_pipelines(VkDevice logical_device, VkPipeline *compute_pipelines, VkPipelineLayout *pipeline_layouts, uint32_t num_compute_shaders, ...);

#endif	// COMPUTE_PIPELINE_H
