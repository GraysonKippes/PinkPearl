#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "descriptor.h"
#include "Pipeline.h"
#include "swapchain.h"

// TODO - move this somewhere else.
VkRenderPass createRenderPass(const VkDevice device, const VkFormat swapchainFormat);

Pipeline createGraphicsPipeline(VkDevice device, Swapchain swapchain, VkRenderPass renderPass, DescriptorSetLayout descriptorSetLayout, VkShaderModule vertex_shader, VkShaderModule fragment_shader);

#endif	// GRAPHICS_PIPELINE_H
