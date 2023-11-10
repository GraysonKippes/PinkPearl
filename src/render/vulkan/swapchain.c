#include "swapchain.h"

#include <stdlib.h>

#include <stdio.h>

#include "log/logging.h"

#define clamp(x, min, max) (x > min ? (x < max ? x : max) : min)

VkSurfaceFormatKHR choose_surface_format(swapchain_support_details_t swapchain_support_details) {

	log_message(VERBOSE, "Selecting surface format for swapchain...");

	for (size_t i = 0; i < swapchain_support_details.m_num_formats; ++i) {
		VkSurfaceFormatKHR format = swapchain_support_details.m_formats[i];
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}

	log_message(ERROR, "No appropriate swapchain image format found.");

	VkSurfaceFormatKHR undefined_format;
	undefined_format.format = VK_FORMAT_UNDEFINED;
	undefined_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	return undefined_format;
}

VkPresentModeKHR choose_present_mode(swapchain_support_details_t swapchain_support_details) {

	log_message(VERBOSE, "Selecting presentation mode for swapchain...");

	for (size_t i = 0; i < swapchain_support_details.m_num_present_modes; ++i) {
		VkPresentModeKHR present_mode = swapchain_support_details.m_present_modes[i];
		if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return present_mode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_extent(swapchain_support_details_t swapchain_support_details, GLFWwindow *window) {

	log_message(VERBOSE, "Selecting extent for swapchain...");

	if (swapchain_support_details.m_capabilities.currentExtent.width != UINT32_MAX) {
		return swapchain_support_details.m_capabilities.currentExtent;
	}
	
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };
	actualExtent.width = clamp(actualExtent.width, swapchain_support_details.m_capabilities.minImageExtent.width, swapchain_support_details.m_capabilities.maxImageExtent.width);
	actualExtent.height = clamp(actualExtent.height, swapchain_support_details.m_capabilities.minImageExtent.height, swapchain_support_details.m_capabilities.maxImageExtent.height);

	return actualExtent;
}

swapchain_t create_swapchain(GLFWwindow *window, VkSurfaceKHR surface, physical_device_t physical_device, VkDevice logical_device, VkSwapchainKHR old_swapchain_handle) {

	log_message(VERBOSE, "Creating swapchain...");

	swapchain_t swapchain;

	VkSurfaceFormatKHR surface_format = choose_surface_format(physical_device.m_swapchain_support_details);
	swapchain.m_image_format = surface_format.format;
	VkPresentModeKHR present_mode = choose_present_mode(physical_device.m_swapchain_support_details);
	swapchain.m_extent = choose_extent(physical_device.m_swapchain_support_details, window);

	// Requested number of images in swap chain.
	uint32_t num_images = physical_device.m_swapchain_support_details.m_capabilities.minImageCount + 1;
	if (physical_device.m_swapchain_support_details.m_capabilities.maxImageCount > 0 && num_images > physical_device.m_swapchain_support_details.m_capabilities.maxImageCount)
		num_images = physical_device.m_swapchain_support_details.m_capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.surface = surface;
	create_info.minImageCount = num_images;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = swapchain.m_extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queue_family_indices[] = { *physical_device.m_queue_family_indices.m_graphics_family_ptr, *physical_device.m_queue_family_indices.m_present_family_ptr };
	if (queue_family_indices[0] != queue_family_indices[1]) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = NULL;
	}

	create_info.preTransform = physical_device.m_swapchain_support_details.m_capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = old_swapchain_handle;

	VkResult swapchain_creation_result = vkCreateSwapchainKHR(logical_device, &create_info, NULL, &swapchain.m_handle);
	if (swapchain_creation_result != VK_SUCCESS) {
		logf_message(FATAL, "Swapchain creation failed. (Error code: %i)", swapchain_creation_result);
		exit(1);
	}

	// Retrieve images
	
	log_message(VERBOSE, "Retrieving images for swapchain...");

	vkGetSwapchainImagesKHR(logical_device, swapchain.m_handle, &num_images, NULL);
	swapchain.m_num_images = num_images;
	swapchain.m_images = malloc(swapchain.m_num_images * sizeof(VkImage));
	vkGetSwapchainImagesKHR(logical_device, swapchain.m_handle, &num_images, swapchain.m_images);

	// Create image views

	log_message(VERBOSE, "Creating image views for swapchain...");

	swapchain.m_image_views = malloc(swapchain.m_num_images * sizeof(VkImageView));
	for (size_t i = 0; i < swapchain.m_num_images; ++i) {

		VkImageViewCreateInfo image_view_create_info = {0};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.pNext = NULL;
		image_view_create_info.flags = 0;
		image_view_create_info.image = swapchain.m_images[i];
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format = swapchain.m_image_format;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;

		VkResult image_view_creation_result = vkCreateImageView(logical_device, &image_view_create_info, NULL, (swapchain.m_image_views + i));
		if (image_view_creation_result != VK_SUCCESS) {
			logf_message(FATAL, "Image view creation for swapchain failed. (Error code: %i)", image_view_creation_result);
			exit(1);
		}
	}

	return swapchain;
}

void create_framebuffers(VkDevice logical_device, VkRenderPass render_pass, swapchain_t *swapchain_ptr) {

	log_message(VERBOSE, "Creating framebuffers for swapchain...");

	swapchain_ptr->m_framebuffers = malloc(swapchain_ptr->m_num_images * sizeof(VkFramebuffer));
	for (uint32_t i = 0; i < swapchain_ptr->m_num_images; ++i) {

		VkFramebufferCreateInfo create_info = {0};
		create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		create_info.pNext = NULL;
		create_info.flags = 0;
		create_info.renderPass = render_pass;
		create_info.attachmentCount = 1;
		create_info.pAttachments = swapchain_ptr->m_image_views + i;
		create_info.width = swapchain_ptr->m_extent.width;
		create_info.height = swapchain_ptr->m_extent.height;
		create_info.layers = 1;

		VkResult result = vkCreateFramebuffer(logical_device, &create_info, NULL, (swapchain_ptr->m_framebuffers + i));
		if (result != VK_SUCCESS) {
			logf_message(FATAL, "Framebuffer creation for swapchain failed. (Error code: %i)", result);
		}
	}
}

void destroy_swapchain(VkDevice logical_device, swapchain_t swapchain) {

	for (size_t i = 0; i < swapchain.m_num_images; ++i) {
		vkDestroyImageView(logical_device, swapchain.m_image_views[i], NULL);
		vkDestroyFramebuffer(logical_device, swapchain.m_framebuffers[i], NULL);
	}

	vkDestroySwapchainKHR(logical_device, swapchain.m_handle, NULL);

	free(swapchain.m_images);
	free(swapchain.m_image_views);
	free(swapchain.m_framebuffers);
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
