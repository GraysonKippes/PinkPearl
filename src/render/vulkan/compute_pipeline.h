#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "descriptor.h"



typedef struct compute_pipeline_t {

	VkPipeline m_handle;
	VkPipelineLayout m_layout;

	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorPool m_descriptor_pool;

	// TODO - look into including a descriptor pool in this struct.

} compute_pipeline_t;



compute_pipeline_t create_compute_pipeline(VkDevice device, descriptor_layout_t descriptor_layout, const char *compute_shader_name);

void destroy_compute_pipeline(VkDevice logical_device, compute_pipeline_t compute_pipeline);



extern const descriptor_layout_t compute_matrices_layout;

extern const descriptor_layout_t compute_room_texture_layout;

extern const descriptor_layout_t compute_textures_layout;



#endif	// COMPUTE_PIPELINE_H
