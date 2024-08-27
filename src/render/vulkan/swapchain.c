#include "swapchain.h"

#include <stdio.h>
#include <stdlib.h>

#include "log/Logger.h"

#include "vulkan_manager.h"

#define clamp(x, min, max) (x > min ? (x < max ? x : max) : min)

VkSurfaceFormatKHR choose_surface_format(SwapchainSupportDetails swapchainSupportDetails) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Selecting surface format for swapchain...");

	for (size_t i = 0; i < swapchainSupportDetails.num_formats; ++i) {
		VkSurfaceFormatKHR format = swapchainSupportDetails.formats[i];
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}

	logMsg(loggerVulkan, LOG_LEVEL_ERROR, "No appropriate swapchain image format found.");

	VkSurfaceFormatKHR undefined_format;
	undefined_format.format = VK_FORMAT_UNDEFINED;
	undefined_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	return undefined_format;
}

VkPresentModeKHR choose_present_mode(SwapchainSupportDetails swapchainSupportDetails) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Selecting presentation mode for swapchain...");

	for (size_t i = 0; i < swapchainSupportDetails.num_present_modes; ++i) {
		VkPresentModeKHR present_mode = swapchainSupportDetails.present_modes[i];
		if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return present_mode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_extent(SwapchainSupportDetails swapchainSupportDetails, GLFWwindow *window) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Selecting extent for swapchain...");

	if (swapchainSupportDetails.capabilities.currentExtent.width != UINT32_MAX) {
		return swapchainSupportDetails.capabilities.currentExtent;
	}
	
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };
	actualExtent.width = clamp(actualExtent.width, swapchainSupportDetails.capabilities.minImageExtent.width, swapchainSupportDetails.capabilities.maxImageExtent.width);
	actualExtent.height = clamp(actualExtent.height, swapchainSupportDetails.capabilities.minImageExtent.height, swapchainSupportDetails.capabilities.maxImageExtent.height);

	return actualExtent;
}

Swapchain createSwapchain(GLFWwindow *window, VkSurfaceKHR surface, PhysicalDevice physical_device, VkDevice device, VkSwapchainKHR old_swapchain_handle) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating swapchain...");

	Swapchain swapchain;

	VkSurfaceFormatKHR surface_format = choose_surface_format(physical_device.swapchainSupportDetails);
	swapchain.image_format = surface_format.format;
	VkPresentModeKHR present_mode = choose_present_mode(physical_device.swapchainSupportDetails);
	swapchain.extent = choose_extent(physical_device.swapchainSupportDetails, window);

	// Requested number of images in swap chain.
	uint32_t num_images = physical_device.swapchainSupportDetails.capabilities.minImageCount + 1;
	if (physical_device.swapchainSupportDetails.capabilities.maxImageCount > 0 && num_images > physical_device.swapchainSupportDetails.capabilities.maxImageCount)
		num_images = physical_device.swapchainSupportDetails.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.surface = surface;
	create_info.minImageCount = num_images;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = swapchain.extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { *physical_device.queueFamilyIndices.graphics_family_ptr, *physical_device.queueFamilyIndices.present_family_ptr };
	if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = physical_device.swapchainSupportDetails.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = old_swapchain_handle;

	VkResult swapchain_creation_result = vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain.handle);
	if (swapchain_creation_result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_FATAL, "Swapchain creation failed. (Error code: %i)", swapchain_creation_result);
		exit(1);
	}

	// Retrieve images
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Retrieving images for swapchain...");

	vkGetSwapchainImagesKHR(device, swapchain.handle, &num_images, nullptr);
	swapchain.num_images = num_images;
	swapchain.images = malloc(swapchain.num_images * sizeof(VkImage));
	vkGetSwapchainImagesKHR(device, swapchain.handle, &num_images, swapchain.images);

	// Create image views

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating image views for swapchain...");

	swapchain.image_views = malloc(swapchain.num_images * sizeof(VkImageView));
	for (size_t i = 0; i < swapchain.num_images; ++i) {

		VkImageViewCreateInfo image_view_create_info = {0};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.pNext = nullptr;
		image_view_create_info.flags = 0;
		image_view_create_info.image = swapchain.images[i];
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format = swapchain.image_format;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;

		VkResult image_view_creation_result = vkCreateImageView(device, &image_view_create_info, nullptr, (swapchain.image_views + i));
		if (image_view_creation_result != VK_SUCCESS) {
			logMsg(loggerVulkan, LOG_LEVEL_FATAL, "Image view creation for swapchain failed. (Error code: %i)", image_view_creation_result);
			exit(1);
		}
	}

	return swapchain;
}

void deleteSwapchain(VkDevice device, Swapchain swapchain) {

	for (size_t i = 0; i < swapchain.num_images; ++i) {
		vkDestroyImageView(device, swapchain.image_views[i], nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain.handle, nullptr);

	free(swapchain.images);
	free(swapchain.image_views);
}

VkViewport make_viewport(VkExtent2D extent) {

	VkViewport viewport;
	viewport.x = 0.0F;
	viewport.y = 0.0F;
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.minDepth = 0.0F;
	viewport.maxDepth = 1.0F;

	return viewport;
}

VkRect2D make_scissor(VkExtent2D extent) {

	VkRect2D scissor;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = extent;

	return scissor;
}
