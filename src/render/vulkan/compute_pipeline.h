#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "descriptor.h"



typedef struct compute_shader_t {

	VkShaderModule m_module;

	// The handle to the descriptor set layout is not destroyed when this shader is destroyed; instead, it is passed onto the compute pipeline.
	VkDescriptorSetLayout m_descriptor_set_layout;

} compute_shader_t;

typedef struct compute_pipeline_t {

	VkPipeline m_handle;
	VkPipelineLayout m_layout;
	VkDescriptorSetLayout m_descriptor_set_layout;

	// TODO - look into including a descriptor pool in this struct.

} compute_pipeline_t;



compute_shader_t create_compute_shader(VkDevice logical_device, descriptor_layout_t descriptor_layout, const char *filename);

void destroy_compute_shader(VkDevice logical_device, compute_shader_t compute_shader);

void create_compute_pipelines(VkDevice logical_device, VkPipeline *compute_pipelines, VkPipelineLayout *pipeline_layouts, uint32_t num_compute_shaders, ...);

void destroy_compute_pipeline(VkDevice logical_device, compute_pipeline_t compute_pipeline);



extern const descriptor_layout_t compute_matrices_layout;

extern const descriptor_layout_t compute_room_texture_layout;

extern const descriptor_layout_t compute_textures_layout;



#endif	// COMPUTE_PIPELINE_H
