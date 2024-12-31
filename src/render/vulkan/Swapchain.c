#include "Swapchain.h"

#include <stdio.h>
#include "log/Logger.h"
#include "util/Allocation.h"
#include "VulkanManager.h"

#define clamp(x, min, max) (x > min ? (x < max ? x : max) : min)

static VkSurfaceFormatKHR selectSurfaceFormat(const SwapchainSupportDetails swapchainSupportDetails);

static VkPresentModeKHR selectPresentMode(const SwapchainSupportDetails swapchainSupportDetails);

static VkExtent2D selectExtent(const SwapchainSupportDetails swapchainSupportDetails, GLFWwindow *const pWindow);

Swapchain createSwapchain(GLFWwindow *const pWindow, const WindowSurface windowSurface, const PhysicalDevice physicalDevice, const VkDevice vkDevice, const VkSwapchainKHR old_swapchain_handle) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating swapchain...");

	Swapchain swapchain = { };

	VkSurfaceFormatKHR surface_format = selectSurfaceFormat(physicalDevice.swapchainSupportDetails);
	VkPresentModeKHR present_mode = selectPresentMode(physicalDevice.swapchainSupportDetails);
	swapchain.imageExtent = selectExtent(physicalDevice.swapchainSupportDetails, pWindow);

	// Requested number of images in swapchain.
	uint32_t imageCount = physicalDevice.swapchainSupportDetails.capabilities.minImageCount + 1;
	if (physicalDevice.swapchainSupportDetails.capabilities.maxImageCount > 0 && imageCount > physicalDevice.swapchainSupportDetails.capabilities.maxImageCount) {
		imageCount = physicalDevice.swapchainSupportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = windowSurface.vkSurface,
		.minImageCount = imageCount,
		.imageFormat = surface_format.format,
		.imageColorSpace = surface_format.colorSpace,
		.imageExtent = swapchain.imageExtent,
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

	const VkResult vkSwapchainCreateResult = vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, nullptr, &swapchain.vkSwapchain);
	if (vkSwapchainCreateResult != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating swapchain: swapchain creation failed (error code: %i).", vkSwapchainCreateResult);
		return swapchain;
	}

	// Retrieve images
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Retrieving images for swapchain...");

	vkGetSwapchainImagesKHR(vkDevice, swapchain.vkSwapchain, &imageCount, nullptr);
	VkImage vkImages[imageCount];
	vkGetSwapchainImagesKHR(vkDevice, swapchain.vkSwapchain, &imageCount, vkImages);
	
	// Create image views

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating image views for swapchain...");

	VkImageView vkImageViews[imageCount];
	for (size_t imageIndex = 0; imageIndex < imageCount; ++imageIndex) {

		const VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = vkImages[imageIndex],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = surface_format.format,
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

		const VkResult imageViewCreateResult = vkCreateImageView(vkDevice, &imageViewCreateInfo, nullptr, &vkImageViews[imageIndex]);
		if (imageViewCreateResult != VK_SUCCESS) {
			logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating swapchain: image view creation for swapchain failed (error code: %i).", imageViewCreateResult);
			return swapchain;
		}
	}
	
	swapchain.pImages = heapAlloc(imageCount, sizeof(Image));
	if (!swapchain.pImages) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating swapchain: failed to allocate array of images.");
		return swapchain;
	}
	
	for (uint32_t imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
		swapchain.pImages[imageIndex] = (Image){
			.usage = imageUsageUndefined,
			.extent = (Extent){
				.width = swapchain.imageExtent.width,
				.length = swapchain.imageExtent.height
			},
			.vkImage = vkImages[imageIndex],
			.vkImageView = vkImageViews[imageIndex],
			.vkFormat = surface_format.format,
			.vkDevice = vkDevice
		};
	}
	
	swapchain.imageCount = imageCount;
	swapchain.imageFormat = surface_format.format;
	swapchain.vkDevice = vkDevice;

	return swapchain;
}

void deleteSwapchain(Swapchain *const pSwapchain) {
	if (!pSwapchain) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting swapchain: pointer to swapchain object is null.");
		return;
	}

	for (size_t imageIndex = 0; imageIndex < pSwapchain->imageCount; ++imageIndex) {
		vkDestroyImageView(pSwapchain->vkDevice, pSwapchain->pImages[imageIndex].vkImageView, nullptr);
	}
	vkDestroySwapchainKHR(pSwapchain->vkDevice, pSwapchain->vkSwapchain, nullptr);
	heapFree(pSwapchain->pImages);
	
	pSwapchain->imageCount = 0;
	pSwapchain->pImages = nullptr;
	pSwapchain->imageFormat = VK_FORMAT_UNDEFINED;
	pSwapchain->imageExtent = (VkExtent2D){ 0, 0 };
	pSwapchain->vkSwapchain = VK_NULL_HANDLE;
	pSwapchain->vkDevice = VK_NULL_HANDLE;
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
