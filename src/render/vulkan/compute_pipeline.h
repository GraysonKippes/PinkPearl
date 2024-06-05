#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "descriptor.h"

typedef struct ComputePipeline {

	VkPipeline handle;
	VkPipelineLayout layout;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;

	VkDevice device;

} ComputePipeline;

ComputePipeline create_compute_pipeline(const VkDevice device, const descriptor_layout_t descriptor_layout, const char *const compute_shader_name);

bool destroy_compute_pipeline(ComputePipeline *const compute_pipeline_ptr);

#endif	// COMPUTE_PIPELINE_H
