#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "descriptor.h"
#include "Pipeline.h"
#include "Shader.h"
#include "swapchain.h"

// TODO - move this somewhere else.
VkRenderPass createRenderPass(const VkDevice vkDevice, const VkFormat swapchainFormat);

Pipeline createGraphicsPipeline(const VkDevice vkDevice, const Swapchain swapchain, const VkRenderPass renderPass, const DescriptorSetLayout descriptorSetLayout, 
		const uint32_t shaderModuleCount, const ShaderModule shaderModules[static const shaderModuleCount]);

#endif	// GRAPHICS_PIPELINE_H
