#include "texture.h"

#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "render/stb/image.h"
#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "VulkanManager.hpp"

static const char *texture_directory = "resources/assets/textures/";

/* -- FUNCTION FORWARD-DECLARATIONS -- */

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

void copyBufferToImage(VkBuffer buffer, VkImage image, size_t width, size_t height);

/* -- FUNCTION DEFINITIONS -- */

texture_t create_texture(const char *path) {
	
	char *full_path;
	strcat(full_path, texture_directory);
	strcat(full_path, path);

	image_t image = load_image(full_path);

	VkDeviceSize image_size = image.m_width * image.m_height * image.m_num_channels;

	VkImageCreateInfo imageInfo;
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.flags = 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = get_image_format(image.m_num_channels);
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.extent.width = static_cast<uint32_t>(image.m_width);
	imageInfo.extent.height = static_cast<uint32_t>(image.m_height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

}

void destroy_texture(texture_t texture) {
	
}

void create_texture_sampler(VkSampler *sampler_ptr) {
	
}

VkFormat get_image_format(size_t num_channels) {
	switch (num_channels) {
		case 1:	return VK_FORMAT_R8_SRGB;
		case 2:	return VK_FORMAT_R8G8_SRGB;
		case 3:	return VK_FORMAT_R8G8B8_SRGB;
		case 4:	return VK_FORMAT_R8G8B8A8_SRGB;
		default: return VK_FORMAT_UNDEFINED;
	}
}





void createTextureImage(const std::string &path, VkImage *pImage, VkDeviceMemory *pMemory)
{
	int width, height, channels;
	stbi_uc *pixels = stbi_load(path.data(), &width, &height, &channels, STBI_rgb_alpha);
	if (!pixels) {
		throw std::runtime_error("failed to load image at " + path);
	}
	VkDeviceSize size = width * height * 4;
	Buffer stagingBuffer = makeStagingBuffer(size, pixels);
	stbi_image_free(pixels);

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.flags = 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	if (vkCreateImage(logicalDevice, &imageInfo, nullptr, pImage) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(logicalDevice, *pImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findPhysicalDeviceMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, pMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory");
	}

	vkBindImageMemory(logicalDevice, *pImage, *pMemory, 0);

	transitionImageLayout(*pImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer.handle, *pImage, (size_t)width, (size_t)height);
	transitionImageLayout(*pImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	destroyBuffer(stagingBuffer);
}

void createTextureImage(Image &image, VkImage *pImage, VkDeviceMemory *pMemory)
{
	VkDeviceSize size = image.getWidth() * image.getHeight() * image.getNumChannels();
	Buffer stagingBuffer = makeStagingBuffer(size, image.data());

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.flags = 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.extent.width = static_cast<uint32_t>(image.getWidth());
	imageInfo.extent.height = static_cast<uint32_t>(image.getHeight());
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	if (vkCreateImage(logicalDevice, &imageInfo, nullptr, pImage) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(logicalDevice, *pImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findPhysicalDeviceMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, pMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory");
	}

	vkBindImageMemory(logicalDevice, *pImage, *pMemory, 0);

	transitionImageLayout(*pImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer.handle, *pImage, image.getWidth(), image.getHeight());
	transitionImageLayout(*pImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	destroyBuffer(stagingBuffer);
}

void createTextureImageView(VkImage image, VkImageView *pImageView)
{
	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, pImageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view");
	}
}

Texture createTexture(const std::string &path, TextureState textureState)
{
	Texture texture(textureState);
	createTextureImage(path, &texture.image, &texture.memory);
	createTextureImageView(texture.image, &texture.view);
	return texture;
}

void destroyTexture(Texture &texture)
{
	vkDestroyImage(logicalDevice, texture.image, nullptr);
	vkFreeMemory(logicalDevice, texture.memory, nullptr);
	vkDestroyImageView(logicalDevice, texture.view, nullptr);
	texture.image = VK_NULL_HANDLE;
	texture.memory = VK_NULL_HANDLE;
	texture.view = VK_NULL_HANDLE;
}

void createTextureSampler(VkSampler *pSampler)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = physicalDevice.properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0F;
	samplerInfo.minLod = 0.0F;
	samplerInfo.maxLod = 0.0F;

	if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, pSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler");
	}
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer;
	createCommandBuffers(commandPool, &commandBuffer, 1);
	beginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition");
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	endCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);
	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, size_t width, size_t height)
{
	VkCommandBuffer commandBuffer;
	createCommandBuffers(commandPool, &commandBuffer, 1);
	beginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);
	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

Image::Image(size_t width, size_t height, size_t channels)
	: m_data((unsigned char *)std::calloc((width * height * channels), 1))
	, m_width(width), m_height(height), m_channels(channels) {}

Image::Image(const std::string &path)
	: m_width(0), m_height(0), m_channels(0)
{
	m_data = stbi_load(path.data(), (int *)(&m_width), (int *)(&m_height), (int *)(&m_channels), STBI_rgb_alpha);
	if (!m_data) {
		throw std::runtime_error("failed to load image at " + path);
	}
}

Image::~Image(void)
{
	std::free(m_data);
}
