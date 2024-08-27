#include "Swapchain.h"

#include <stdio.h>
#include <stdlib.h>

#include "log/Logger.h"

#include "vulkan_manager.h"

#define clamp(x, min, max) (x > min ? (x < max ? x : max) : min)



static VkSurfaceFormatKHR selectSurfaceFormat(const SwapchainSupportDetails swapchainSupportDetails);

static VkPresentModeKHR selectPresentMode(const SwapchainSupportDetails swapchainSupportDetails);

static VkExtent2D selectExtent(const SwapchainSupportDetails swapchainSupportDetails, GLFWwindow *const pWindow);

Swapchain createSwapchain(GLFWwindow *const pWindow, const WindowSurface windowSurface, const PhysicalDevice physicalDevice, const VkDevice vkDevice, const VkSwapchainKHR old_swapchain_handle) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating swapchain...");

	Swapchain swapchain = { };

	VkSurfaceFormatKHR surface_format = selectSurfaceFormat(physicalDevice.swapchainSupportDetails);
	swapchain.image_format = surface_format.format;
	VkPresentModeKHR present_mode = selectPresentMode(physicalDevice.swapchainSupportDetails);
	swapchain.extent = selectExtent(physicalDevice.swapchainSupportDetails, pWindow);

	// Requested number of images in swapchain.
	uint32_t num_images = physicalDevice.swapchainSupportDetails.capabilities.minImageCount + 1;
	if (physicalDevice.swapchainSupportDetails.capabilities.maxImageCount > 0 && num_images > physicalDevice.swapchainSupportDetails.capabilities.maxImageCount) {
		num_images = physicalDevice.swapchainSupportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = windowSurface.vkSurface,
		.minImageCount = num_images,
		.imageFormat = surface_format.format,
		.imageColorSpace = surface_format.colorSpace,
		.imageExtent = swapchain.extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = physicalDevice.swapchainSupportDetails.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = old_swapchain_handle
	};

	uint32_t queueFamilyIndices[] = { *physicalDevice.queueFamilyIndices.graphics_family_ptr, *physicalDevice.queueFamilyIndices.present_family_ptr };
	if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	const VkResult swapchain_creation_result = vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, nullptr, &swapchain.vkSwapchain);
	if (swapchain_creation_result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating swapchain: swapchain creation failed (error code: %i).", swapchain_creation_result);
		return swapchain;
	}

	// Retrieve images
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Retrieving images for swapchain...");

	vkGetSwapchainImagesKHR(vkDevice, swapchain.vkSwapchain, &num_images, nullptr);
	swapchain.num_images = num_images;
	swapchain.images = malloc(swapchain.num_images * sizeof(VkImage));
	vkGetSwapchainImagesKHR(vkDevice, swapchain.vkSwapchain, &num_images, swapchain.images);

	// Create image views

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating image views for swapchain...");

	swapchain.image_views = malloc(swapchain.num_images * sizeof(VkImageView));
	for (size_t i = 0; i < swapchain.num_images; ++i) {

		const VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = swapchain.images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapchain.image_format,
			.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1
		};

		VkResult imageViewCreateResult = vkCreateImageView(vkDevice, &imageViewCreateInfo, nullptr, &swapchain.image_views[i]);
		if (imageViewCreateResult != VK_SUCCESS) {
			logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating swapchain: image view creation for swapchain failed (error code: %i).", imageViewCreateResult);
			return swapchain;
		}
	}
	
	swapchain.vkDevice = vkDevice;

	return swapchain;
}

void deleteSwapchain(Swapchain *const pSwapchain) {
	if (!pSwapchain) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting swapchain: pointer to swapchain object is null.");
		return;
	}

	for (size_t i = 0; i < swapchain.num_images; ++i) {
		vkDestroyImageView(pSwapchain->vkDevice, pSwapchain->image_views[i], nullptr);
	}
	vkDestroySwapchainKHR(pSwapchain->vkDevice, pSwapchain->vkSwapchain, nullptr);
	free(pSwapchain->images);
	free(pSwapchain->image_views);
	
	pSwapchain->vkSwapchain = VK_NULL_HANDLE;
	pSwapchain->vkDevice = VK_NULL_HANDLE;
	pSwapchain->image_format = VK_FORMAT_UNDEFINED;
	pSwapchain->extent = (VkExtent2D){ 0, 0 };
	pSwapchain->num_images = 0;
	pSwapchain->images = nullptr;
	pSwapchain->image_views = nullptr;
}

VkViewport makeViewport(const VkExtent2D extent) {
	return (VkViewport){
		.x = 0.0F,
		.y = 0.0F,
		.width = (float)extent.width,
		.height = (float)extent.height,
		.minDepth = 0.0F,
		.maxDepth = 1.0F
	};
}

VkRect2D makeScissor(const VkExtent2D extent) {
	return (VkRect2D){
		.offset.x = 0,
		.offset.y = 0,
		.extent = extent
	};
}

static VkSurfaceFormatKHR selectSurfaceFormat(const SwapchainSupportDetails swapchainSupportDetails) {

	for (size_t i = 0; i < swapchainSupportDetails.num_formats; ++i) {
		VkSurfaceFormatKHR format = swapchainSupportDetails.formats[i];
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}

	logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating swapchain: no appropriate swapchain image format found.");

	return (VkSurfaceFormatKHR){
		.format = VK_FORMAT_UNDEFINED,
		.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	};
}

static VkPresentModeKHR selectPresentMode(const SwapchainSupportDetails swapchainSupportDetails) {
	
	for (size_t i = 0; i < swapchainSupportDetails.num_present_modes; ++i) {
		VkPresentModeKHR present_mode = swapchainSupportDetails.present_modes[i];
		if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return present_mode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D selectExtent(const SwapchainSupportDetails swapchainSupportDetails, GLFWwindow *const pWindow) {
	
	if (swapchainSupportDetails.capabilities.currentExtent.width != UINT32_MAX) {
		return swapchainSupportDetails.capabilities.currentExtent;
	}
	
	int width = 0, height = 0;
	glfwGetFramebufferSize(pWindow, &width, &height);

	VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };
	actualExtent.width = clamp(actualExtent.width, swapchainSupportDetails.capabilities.minImageExtent.width, swapchainSupportDetails.capabilities.maxImageExtent.width);
	actualExtent.height = clamp(actualExtent.height, swapchainSupportDetails.capabilities.minImageExtent.height, swapchainSupportDetails.capabilities.maxImageExtent.height);

	return actualExtent;
}
