#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "descriptor.h"
#include "Pipeline.h"

Pipeline createComputePipeline(const VkDevice vkDevice, const DescriptorSetLayout descriptorSetLayout, const char *const pComputeShaderFilename);

#endif	// COMPUTE_PIPELINE_H
