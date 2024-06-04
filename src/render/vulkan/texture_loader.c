#include "texture_loader.h"

#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "config.h"
#include "log/logging.h"
#include "render/stb/image_data.h"
#include "util/allocate.h"

#include "buffer.h"
#include "command_buffer.h"
#include "vulkan_manager.h"

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

	// TODO - modify this to use semaphores between image transitions and data transfer operations.

	if (stringIsNull(textureCreateInfo.textureID)) {
		log_message(ERROR, "Error loading texture: texture ID is null.");
		return makeNullTexture();
	}

	logf_message(VERBOSE, "Loading texture \"%s\"...", textureCreateInfo.textureID.buffer);

	if (textureCreateInfo.numAnimations == 0) {
		log_message(ERROR, "Error loading texture: number of animation create infos is zero.");
	}

	if (textureCreateInfo.numAnimations > 0 && textureCreateInfo.animations == NULL) {
		log_message(ERROR, "Error loading texture: number of animation create infos is greater than zero, but array of animation create infos is NULL.");
		return makeNullTexture();
	}

	Texture texture = createTexture(textureCreateInfo);

	// Create semaphores.
	VkSemaphore semaphore_transition_finished = VK_NULL_HANDLE;
	VkSemaphore semaphore_transfer_finished = VK_NULL_HANDLE;

	const VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

	vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &semaphore_transition_finished);
	vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &semaphore_transfer_finished);

	// Create full path.
	const String path = textureIDToPath(textureCreateInfo.textureID);

	// Load image data into image staging buffer.
	byte_t *const mapped_memory = buffer_partition_map_memory(global_staging_buffer_partition, 2);
	image_data_t base_image_data = load_image_data(path.buffer, 0);
	const VkDeviceSize base_image_width = base_image_data.width;
	const VkDeviceSize base_image_height = base_image_data.height;
	const VkDeviceSize base_image_channels = base_image_data.num_channels;
	const VkDeviceSize base_image_size = base_image_width * base_image_height * base_image_channels;
	memcpy(mapped_memory, base_image_data.data, base_image_size);
	free_image_data(base_image_data);
	buffer_partition_unmap_memory(global_staging_buffer_partition);
			
	// Subresource range used in all image views and layout transitions.	
	static const TextureImageSubresourceRange imageSubresourceRange = {
		.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};

	// Command buffer for first image layout transition (undefined to transfer destination).
	VkCommandBuffer transitionCommandBuffer1 = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transitionCommandBuffer1);
	begin_command_buffer(transitionCommandBuffer1, 0); {
		
		const VkImageMemoryBarrier2 imageMemoryBarrier = makeImageTransitionBarrier(texture.image, imageSubresourceRange, imageUsageTransferDestination);
		
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
		
		vkCmdPipelineBarrier2(transitionCommandBuffer1, &dependencyInfo);

	} vkEndCommandBuffer(transitionCommandBuffer1);

	{	// First submit
		const VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = NULL,
			.pWaitDstStageMask = NULL,
			.commandBufferCount = 1,
			.pCommandBuffers = &transitionCommandBuffer1,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &semaphore_transition_finished
		};
		vkQueueSubmit(graphics_queue, 1, &submitInfo, NULL);
	}

	texture.image.usage = imageUsageTransferDestination;

	// Transfer image data to texture images.
	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, transfer_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); {
		
		const uint32_t numBufImgCopies = texture.numImageArrayLayers;
		VkBufferImageCopy2 *bufImgCopies = NULL;
		if (!allocate((void **)&bufImgCopies, numBufImgCopies, sizeof(VkBufferImageCopy2))) {
			log_message(ERROR, "Error loading texture: failed to allocate copy region pointer-array.");
			// TODO - do proper cleanup here.
			return texture;
		}

		const VkDeviceSize buffer_partition_offset = global_staging_buffer_partition.ranges[2].offset;
		for (uint32_t i = 0; i < numBufImgCopies; ++i) {
			bufImgCopies[i].sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
			bufImgCopies[i].pNext = NULL;

			const uint32_t atlasExtentWidth = textureCreateInfo.numCells.width * textureCreateInfo.cellExtent.width;
			const uint32_t atlasExtentLength = textureCreateInfo.numCells.length * textureCreateInfo.cellExtent.length;

			const uint32_t cell_offset = i;
			const uint32_t cell_offset_x = cell_offset % textureCreateInfo.numCells.width;
			const uint32_t cell_offset_y = cell_offset / textureCreateInfo.numCells.width;

			const uint32_t texel_offset_x = cell_offset_x * textureCreateInfo.cellExtent.width;
			const uint32_t texel_offset_y = cell_offset_y * textureCreateInfo.cellExtent.length;
			const uint32_t texel_offset = texel_offset_y * atlasExtentWidth + texel_offset_x;

			static const VkDeviceSize bytes_per_texel = 4;

			bufImgCopies[i].bufferOffset = buffer_partition_offset + (VkDeviceSize)texel_offset * bytes_per_texel;
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
			.pNext = NULL,
			.srcBuffer = global_staging_buffer_partition.buffer,
			.dstImage = texture.image.vkImage,
			.dstImageLayout = texture.image.usage.imageLayout,
			.regionCount = numBufImgCopies,
			.pRegions = bufImgCopies
		};

		vkCmdCopyBufferToImage2(transfer_command_buffer, &copy_info);

		deallocate((void **)&bufImgCopies);
	} vkEndCommandBuffer(transfer_command_buffer);

	VkPipelineStageFlags transfer_stage_flags[1] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	{	// Second submit
		const VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &semaphore_transition_finished,
			.pWaitDstStageMask = transfer_stage_flags,
			.commandBufferCount = 1,
			.pCommandBuffers = &transfer_command_buffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &semaphore_transfer_finished
		};
		vkQueueSubmit(transfer_queue, 1, &submitInfo, NULL);
	}

	// Command buffer for second image layout transition (transfer destination to sampled).
	VkCommandBuffer transitionCommandBuffer2 = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transitionCommandBuffer2);
	begin_command_buffer(transitionCommandBuffer2, 0); {

		TextureImageUsage imageUsage = imageUsageSampled;
		if (textureCreateInfo.isTilemap) {
			imageUsage = imageUsageComputeRead;
		}

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
		
		vkCmdPipelineBarrier2(transitionCommandBuffer2, &dependencyInfo);
		
		texture.image.usage = imageUsage;

	} vkEndCommandBuffer(transitionCommandBuffer2);

	VkPipelineStageFlags transition_1_stage_flags[1] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	{	// Third submit
		const VkSubmitInfo submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &semaphore_transfer_finished,
			.pWaitDstStageMask = transition_1_stage_flags,
			.commandBufferCount = 1,
			.pCommandBuffers = &transitionCommandBuffer2,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = NULL
		};
		vkQueueSubmit(graphics_queue, 1, &submit_info, NULL);
	}

	vkQueueWaitIdle(graphics_queue);
	vkQueueWaitIdle(transfer_queue);

	vkFreeCommandBuffers(device, render_command_pool, 1, &transitionCommandBuffer1);
	vkFreeCommandBuffers(device, render_command_pool, 1, &transitionCommandBuffer2);
	vkFreeCommandBuffers(device, transfer_command_pool, 1, &transfer_command_buffer);

	vkDestroySemaphore(device, semaphore_transition_finished, NULL);
	vkDestroySemaphore(device, semaphore_transfer_finished, NULL);

	return texture;
}
