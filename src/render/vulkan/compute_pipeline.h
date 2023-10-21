#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

typedef struct compute_shader_t {

	VkShaderModule m_module;
	VkDescriptorSetLayout m_descriptor_set_layout;

} compute_shader_t;

void create_compute_pipelines(VkDevice logical_device, VkPipeline *compute_pipelines, uint32_t num_compute_shaders, ...);

#endif	// COMPUTE_PIPELINE_H
