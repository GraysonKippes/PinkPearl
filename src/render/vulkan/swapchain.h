#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <stddef.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "physical_device.h"

typedef struct Swapchain {
	
	VkSwapchainKHR handle;

	VkFormat image_format;
	VkExtent2D extent;

	uint32_t num_images;
	VkImage *images;
	VkImageView *image_views;
	VkFramebuffer *framebuffers;

} Swapchain;

Swapchain create_swapchain(GLFWwindow *window, VkSurfaceKHR surface, physical_device_t physical_device, VkDevice device, VkSwapchainKHR old_swapchain_handle);

void create_framebuffers(VkDevice device, VkRenderPass render_pass, Swapchain *swapchain_ptr);

void destroy_swapchain(VkDevice device, Swapchain swapchain);

VkViewport make_viewport(VkExtent2D extent);

VkRect2D make_scissor(VkExtent2D extent);

#endif	// SWAPCHAIN_H
