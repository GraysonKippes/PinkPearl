#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "descriptor.h"

typedef struct compute_pipeline_t {

	VkPipeline handle;
	VkPipelineLayout layout;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;

	VkDevice device;

} compute_pipeline_t;

compute_pipeline_t create_compute_pipeline(const VkDevice device, const descriptor_layout_t descriptor_layout, const char *const compute_shader_name);

bool destroy_compute_pipeline(compute_pipeline_t *const compute_pipeline_ptr);

#endif	// COMPUTE_PIPELINE_H
