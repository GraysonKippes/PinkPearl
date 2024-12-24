#include "texture_loader.h"

#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "config.h"
#include "log/Logger.h"
#include "render/stb/ImageData.h"
#include "util/allocate.h"
#include "buffer.h"
#include "CommandBuffer.h"
#include "VulkanManager.h"

#define TEXTURE_PATH (RESOURCE_PATH "assets/textures/")

static String textureIDToPath(const String textureID) {
	
	String path = newStringEmpty(256);
	
	if (stringIsNull(textureID)) {
		stringConcatCString(&path, TEXTURE_PATH);
		stringConcatCString(&path, "missing.png");
	} else {
		stringConcatCString(&path, TEXTURE_PATH);
		stringConcatString(&path, textureID);
		stringConcatCString(&path, ".png");
	}
	
	return path;
}

// Loaded textures are required to have animation create infos all with the same extent.
Texture loadTexture(const TextureCreateInfo textureCreateInfo) {

	static const VkDeviceSize numImageChannels = 4;

	// TODO: modify this to use semaphores between image transitions and data transfer operations.

	if (stringIsNull(textureCreateInfo.textureID)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error loading texture: texture ID is null.");
		return nullTexture;
	}

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Loading texture \"%s\"...", textureCreateInfo.textureID.pBuffer);

	if (textureCreateInfo.numAnimations == 0) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error loading texture: number of animation create infos is zero.");
	}

	if (textureCreateInfo.numAnimations > 0 && textureCreateInfo.animations == nullptr) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error loading texture: number of animation create infos is greater than zero, but array of animation create infos is nullptr.");
		return nullTexture;
	}

	Texture texture = createTexture(textureCreateInfo);

	// Create semaphores.
	VkSemaphore semaphore_transition_finished = VK_NULL_HANDLE;
	VkSemaphore semaphore_transfer_finished = VK_NULL_HANDLE;
	const VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0
	};
	vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore_transition_finished);
	vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore_transfer_finished);

	// Create full path.
	const String path = textureIDToPath(textureCreateInfo.textureID);

	// Load image data into image staging buffer.
	byte_t *const mapped_memory = buffer_partition_map_memory(global_staging_buffer_partition, 2);
	ImageData base_image_data = loadImageData(path.pBuffer, numImageChannels);
	const VkDeviceSize base_image_width = base_image_data.width;
	const VkDeviceSize base_image_height = base_image_data.height;
	const VkDeviceSize base_image_size = base_image_width * base_image_height * numImageChannels;
	memcpy(mapped_memory, base_image_data.pPixels, base_image_size);
	deleteImageData(&base_image_data);
	buffer_partition_unmap_memory(global_staging_buffer_partition);
	
	// Subresource range used in all image views and layout transitions.	
	static const ImageSubresourceRange imageSubresourceRange = {
		.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseArrayLayer = 0,
		.arrayLayerCount = VK_REMAINING_ARRAY_LAYERS
	};

	// Transfer image data to texture images.
	CmdBufArray transferCmdBufs = cmdBufAlloc(commandPoolTransfer, 1);
	recordCommands(transferCmdBufs, 0, true,
		
		const uint32_t numBufImgCopies = texture.image.arrayLayerCount;
		VkBufferImageCopy2 *bufImgCopies = nullptr;
		if (!allocate((void **)&bufImgCopies, numBufImgCopies, sizeof(VkBufferImageCopy2))) {
			logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error loading texture: failed to allocate copy region pointer-array.");
			// TODO - do proper cleanup here.
			return texture;
		}

		const VkDeviceSize buffer_partition_offset = global_staging_buffer_partition.ranges[2].offset;
		for (uint32_t i = 0; i < numBufImgCopies; ++i) {
			bufImgCopies[i].sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
			bufImgCopies[i].pNext = nullptr;

			const uint32_t atlasExtentWidth = textureCreateInfo.numCells.width * textureCreateInfo.cellExtent.width;
			const uint32_t atlasExtentLength = textureCreateInfo.numCells.length * textureCreateInfo.cellExtent.length;

			const uint32_t cell_offset = i;
			const uint32_t cell_offset_x = cell_offset % textureCreateInfo.numCells.width;
			const uint32_t cell_offset_y = cell_offset / textureCreateInfo.numCells.width;

			const uint32_t texel_offset_x = cell_offset_x * textureCreateInfo.cellExtent.width;
			const uint32_t texel_offset_y = cell_offset_y * textureCreateInfo.cellExtent.length;
			const uint32_t texel_offset = texel_offset_y * atlasExtentWidth + texel_offset_x;

			bufImgCopies[i].bufferOffset = buffer_partition_offset + (VkDeviceSize)texel_offset * numImageChannels;
			bufImgCopies[i].bufferRowLength = atlasExtentWidth;
			bufImgCopies[i].bufferImageHeight = atlasExtentLength;

			bufImgCopies[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufImgCopies[i].imageSubresource.mipLevel = 0;
			bufImgCopies[i].imageSubresource.baseArrayLayer = i;
			bufImgCopies[i].imageSubresource.layerCount = 1;

			bufImgCopies[i].imageOffset.x = 0;
			bufImgCopies[i].imageOffset.y = 0;
			bufImgCopies[i].imageOffset.z = 0;

			bufImgCopies[i].imageExtent.width = textureCreateInfo.cellExtent.width;
			bufImgCopies[i].imageExtent.height = textureCreateInfo.cellExtent.length;
			bufImgCopies[i].imageExtent.depth = 1;
		}

		const VkCopyBufferToImageInfo2 copy_info = {
			.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
			.pNext = nullptr,
			.srcBuffer = global_staging_buffer_partition.buffer,
			.dstImage = texture.image.vkImage,
			.dstImageLayout = texture.image.usage.imageLayout,
			.regionCount = numBufImgCopies,
			.pRegions = bufImgCopies
		};

		vkCmdCopyBufferToImage2(cmdBuf, &copy_info);

		deallocate((void **)&bufImgCopies);
	);

	{	// Second submit // TODO: use vkQueueSubmit2
		const VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &transferCmdBufs.pCmdBufs[0],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &semaphore_transfer_finished
		};
		vkQueueSubmit(queueTransfer, 1, &submitInfo, nullptr);
	}

	// Command buffer for second image layout transition (transfer destination to sampled).
	CmdBufArray transitionCmdBufs = cmdBufAlloc(commandPoolGraphics, 1);
	recordCommands(transitionCmdBufs, 0, true,
		const ImageUsage imageUsage = textureCreateInfo.isTilemap ? imageUsageComputeRead : imageUsageSampled;
		const VkImageMemoryBarrier2 imageMemoryBarrier = makeImageTransitionBarrier(texture.image, imageSubresourceRange, imageUsage);
		const VkDependencyInfo dependencyInfo = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.dependencyFlags = 0,
			.memoryBarrierCount = 0,
			.pMemoryBarriers = nullptr,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers = nullptr,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &imageMemoryBarrier
		};
		vkCmdPipelineBarrier2(cmdBuf, &dependencyInfo);
		texture.image.usage = imageUsage;
	);

	VkPipelineStageFlags transition_1_stage_flags[1] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	{	// Third submit // TODO: use vkQueueSubmit2
		const VkSubmitInfo submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &semaphore_transfer_finished,
			.pWaitDstStageMask = transition_1_stage_flags,
			.commandBufferCount = 1,
			.pCommandBuffers = &transitionCmdBufs.pCmdBufs[0],
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr
		};
		vkQueueSubmit(queueGraphics, 1, &submit_info, nullptr);
	}

	vkQueueWaitIdle(queueGraphics);
	vkQueueWaitIdle(queueTransfer);
	cmdBufFree(&transferCmdBufs);
	cmdBufFree(&transitionCmdBufs);
	vkDestroySemaphore(device, semaphore_transition_finished, nullptr);
	vkDestroySemaphore(device, semaphore_transfer_finished, nullptr);

	return texture;
}
