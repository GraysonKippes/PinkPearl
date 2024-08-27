#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "descriptor.h"
#include "Pipeline.h"
#include "Shader.h"
#include "swapchain.h"

Pipeline createGraphicsPipeline(const VkDevice vkDevice, const Swapchain swapchain, const DescriptorSetLayout descriptorSetLayout,
		const VkPrimitiveTopology topology, const VkPolygonMode polygonMode,
		const uint32_t shaderModuleCount, const ShaderModule shaderModules[static const shaderModuleCount]);

#endif	// GRAPHICS_PIPELINE_H
