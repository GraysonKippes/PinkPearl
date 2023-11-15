#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <stddef.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "physical_device.h"

typedef struct swapchain_t {
	
	VkSwapchainKHR m_handle;

	VkFormat m_image_format;
	VkExtent2D m_extent;

	uint32_t m_num_images;
	VkImage *m_images;
	VkImageView *m_image_views;
	VkFramebuffer *m_framebuffers;

} swapchain_t;

swapchain_t create_swapchain(GLFWwindow *window, VkSurfaceKHR surface, physical_device_t physical_device, VkDevice device, VkSwapchainKHR old_swapchain_handle);

void create_framebuffers(VkDevice device, VkRenderPass render_pass, swapchain_t *swapchain_ptr);

void destroy_swapchain(VkDevice device, swapchain_t swapchain);

VkViewport make_viewport(VkExtent2D extent);

VkRect2D make_scissor(VkExtent2D extent);

#endif	// SWAPCHAIN_H
