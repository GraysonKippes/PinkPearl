#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "descriptor.h"



typedef struct compute_pipeline_t {

	VkPipeline handle;
	VkPipelineLayout layout;

	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;

	// TODO - look into including a descriptor pool in this struct.

} compute_pipeline_t;



compute_pipeline_t create_compute_pipeline(VkDevice device, descriptor_layout_t descriptor_layout, const char *compute_shader_name);

void destroy_compute_pipeline(VkDevice device, compute_pipeline_t compute_pipeline);



extern const descriptor_layout_t compute_matrices_layout;



#endif	// COMPUTE_PIPELINE_H
