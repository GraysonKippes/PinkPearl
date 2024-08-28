#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <stddef.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "physical_device.h"
#include "texture.h"
#include "vulkan_instance.h"

// The swapchain contains an array of images to render to and present to the window surface.
typedef struct Swapchain {
	
	// The array of images of this swapchain.
	uint32_t imageCount;
	Image *pImages;

	// The format of all the images in this swapchain.
	VkFormat imageFormat;
	
	// The dimensions of all the images in this swapchain.
	VkExtent2D imageExtent;
	
	// The handle to the Vulkan swapchain object of this swapchain.
	VkSwapchainKHR vkSwapchain;
	
	// The handle to the Vulkan device with which this swapchain was created.
	VkDevice vkDevice;

} Swapchain;

Swapchain createSwapchain(GLFWwindow *const pWindow, const WindowSurface windowSurface, const PhysicalDevice physicalDevice, const VkDevice vkDevice, const VkSwapchainKHR old_swapchain_handle);

void deleteSwapchain(Swapchain *const pSwapchain);

VkViewport makeViewport(const VkExtent2D extent);

VkRect2D makeScissor(const VkExtent2D extent);

#endif	// SWAPCHAIN_H
