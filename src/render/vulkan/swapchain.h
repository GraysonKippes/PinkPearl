#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <stddef.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "physical_device.h"
#include "vulkan_instance.h"

typedef struct Swapchain {
	
	VkSwapchainKHR vkSwapchain;
	
	VkDevice vkDevice;

	VkFormat image_format;
	VkExtent2D extent;

	uint32_t num_images;
	VkImage *images;
	VkImageView *image_views;

} Swapchain;

Swapchain createSwapchain(GLFWwindow *const pWindow, const WindowSurface windowSurface, const PhysicalDevice physicalDevice, const VkDevice vkDevice, const VkSwapchainKHR old_swapchain_handle);

void deleteSwapchain(Swapchain *const pSwapchain);

VkViewport makeViewport(const VkExtent2D extent);

VkRect2D makeScissor(const VkExtent2D extent);

#endif	// SWAPCHAIN_H
