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



#define TEXTURE_NAME_MAX_LENGTH 256

static const size_t texture_name_max_length = TEXTURE_NAME_MAX_LENGTH;

#define TEXTURE_PATH (RESOURCE_PATH "assets/textures/")



static void make_image_path(const char *const info_path, char *const image_path) {

	if (info_path == NULL || image_path == NULL) {
		return;
	}

	memset(image_path, '\0', texture_name_max_length);

	const size_t directory_length = strnlen_s(TEXTURE_PATH, texture_name_max_length);
	const errno_t directory_strncpy_result = strncpy_s(image_path, 256, TEXTURE_PATH, directory_length);
	if (directory_strncpy_result != 0) {
		logf_message(WARNING, "Warning loading texture: failed to copy directory into image path buffer. (Error code: %u)", directory_strncpy_result);
	}

	const size_t filename_length = strnlen_s(info_path, 64);
	const errno_t filename_strncat_result = strncat_s(image_path, 256, info_path, filename_length);
	if (filename_strncat_result != 0) {
		logf_message(WARNING, "Warning loading texture: failed to concatenate image filename into image path buffer. (Error code: %u)", filename_strncat_result);
	}
}

// Loaded textures are required to have animation create infos all with the same extent.
Texture loadTexture(const texture_create_info_t texture_create_info) {

	// TODO - modify this to use semaphores between image transitions and data transfer operations.

	logf_message(VERBOSE, "Loading texture at \"%s\"...", texture_create_info.path);

	if (texture_create_info.path == NULL) {
		log_message(ERROR, "Error loading texture: texture path is NULL.");
		return make_null_texture();
	}

	if (texture_create_info.num_animations == 0) {
		log_message(ERROR, "Error loading texture: number of animation create infos is zero.");
	}

	if (texture_create_info.num_animations > 0 && texture_create_info.animations == NULL) {
		log_message(ERROR, "Error loading texture: number of animation create infos is greater than zero, but array of animation create infos is NULL.");
		return make_null_texture();
	}

	/* NOT NEEDED ANYMORE
	const extent_t first_cell_extent = texture_create_info.cell_extent;
	for (uint32_t i = 1; i < texture_create_info.num_animations; ++i) {
		const extent_t test_cell_extent = texture_create_info.animations[i].cell_extent;
		if (!extent_equals(first_cell_extent, test_cell_extent)) {
			logf_message(ERROR, "Error loading texture: animation %u cell extent (%u, %u) is different from first animation cell extent (%u, %u).",
					i, test_cell_extent.width, test_cell_extent.length, first_cell_extent.width, first_cell_extent.length);
			return make_null_texture();
		}
	} */

	Texture texture = create_texture(texture_create_info);

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
	char path[TEXTURE_NAME_MAX_LENGTH];
	make_image_path(texture_create_info.path, path);

	// Load image data into image staging buffer.
	byte_t *const mapped_memory = buffer_partition_map_memory(global_staging_buffer_partition, 2);
	image_data_t base_image_data = load_image_data(path, 0);
	const VkDeviceSize base_image_width = base_image_data.width;
	const VkDeviceSize base_image_height = base_image_data.height;
	const VkDeviceSize base_image_channels = base_image_data.num_channels;
	const VkDeviceSize base_image_size = base_image_width * base_image_height * base_image_channels;
	memcpy(mapped_memory, base_image_data.data, base_image_size);
	free_image_data(base_image_data);
	buffer_partition_unmap_memory(global_staging_buffer_partition);

	// Subresource range used in all image views and layout transitions.
	static const VkImageSubresourceRange image_subresource_range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};

	// Command buffer for first image layout transition (undefined to transfer destination).
	VkCommandBuffer transition_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transition_command_buffer);
	begin_command_buffer(transition_command_buffer, 0); {
	
		const VkImageMemoryBarrier image_memory_barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = NULL,
			.srcAccessMask = VK_ACCESS_NONE,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = texture.image.vkImage,
			.subresourceRange = image_subresource_range
		};

		const VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		const VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		vkCmdPipelineBarrier(transition_command_buffer, source_stage, destination_stage, 0,
				0, NULL,
				0, NULL,
				1, &image_memory_barrier);

	} vkEndCommandBuffer(transition_command_buffer);

	{	// First submit
		const VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = NULL,
			.pWaitDstStageMask = NULL,
			.commandBufferCount = 1,
			.pCommandBuffers = &transition_command_buffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &semaphore_transition_finished
		};
		vkQueueSubmit(graphics_queue, 1, &submitInfo, NULL);
	}

	texture.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	// Transfer image data to texture images.
	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, transfer_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); {
		
		const uint32_t num_copy_regions = texture.numImageArrayLayers;
		VkBufferImageCopy2 *copy_regions = NULL;
		if (!allocate((void **)&copy_regions, num_copy_regions, sizeof(VkBufferImageCopy2))) {
			log_message(ERROR, "Error loading texture: failed to allocate copy region pointer-array.");
			// TODO - do proper cleanup here.
			return texture;
		}

		const VkDeviceSize buffer_partition_offset = global_staging_buffer_partition.ranges[2].offset;
		for (uint32_t i = 0; i < num_copy_regions; ++i) {

			copy_regions[i].sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
			copy_regions[i].pNext = NULL;

			const uint32_t cell_offset = i;
			const uint32_t cell_offset_x = cell_offset % texture_create_info.num_cells.width;
			const uint32_t cell_offset_y = cell_offset / texture_create_info.num_cells.width;

			const uint32_t texel_offset_x = cell_offset_x * texture_create_info.cell_extent.width;
			const uint32_t texel_offset_y = cell_offset_y * texture_create_info.cell_extent.length;
			const uint32_t texel_offset = texel_offset_y * texture_create_info.atlas_extent.width + texel_offset_x;

			static const VkDeviceSize bytes_per_texel = 4;

			copy_regions[i].bufferOffset = buffer_partition_offset + (VkDeviceSize)texel_offset * bytes_per_texel;
			copy_regions[i].bufferRowLength = texture_create_info.atlas_extent.width;
			copy_regions[i].bufferImageHeight = texture_create_info.atlas_extent.length;

			copy_regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_regions[i].imageSubresource.mipLevel = 0;
			copy_regions[i].imageSubresource.baseArrayLayer = i;
			copy_regions[i].imageSubresource.layerCount = 1;

			copy_regions[i].imageOffset.x = 0;
			copy_regions[i].imageOffset.y = 0;
			copy_regions[i].imageOffset.z = 0;

			copy_regions[i].imageExtent.width = texture_create_info.cell_extent.width;
			copy_regions[i].imageExtent.height = texture_create_info.cell_extent.length;
			copy_regions[i].imageExtent.depth = 1;
		}

		const VkCopyBufferToImageInfo2 copy_info = {
			.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
			.pNext = NULL,
			.srcBuffer = global_staging_buffer_partition.buffer,
			.dstImage = texture.image.vkImage,
			.dstImageLayout = texture.layout,
			.regionCount = num_copy_regions,
			.pRegions = copy_regions
		};

		vkCmdCopyBufferToImage2(transfer_command_buffer, &copy_info);

		deallocate((void **)&copy_regions);
	} vkEndCommandBuffer(transfer_command_buffer);

	VkPipelineStageFlags transfer_stage_flags[1] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	{	// Second submit
		const VkSubmitInfo submit_info = {
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

		vkQueueSubmit(transfer_queue, 1, &submit_info, NULL);
	}

	// Command buffer for second image layout transition (transfer destination to sampled).
	VkCommandBuffer transition_1_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transition_1_command_buffer);
	begin_command_buffer(transition_1_command_buffer, 0); {
		
		VkImageMemoryBarrier image_memory_barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = NULL,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = texture.image.vkImage,
			.subresourceRange = image_subresource_range,
		};

		const VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		// If this is a tilemap texture, transition it to the general layout instead 
		// 	so that it can be read by the room texture compute shader.
		if (texture_create_info.type == TEXTURE_TYPE_TILEMAP) {
			image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		vkCmdPipelineBarrier(transition_1_command_buffer, source_stage, destination_stage, 0,
				0, NULL,
				0, NULL,
				1, &image_memory_barrier);

	} vkEndCommandBuffer(transition_1_command_buffer);

	VkPipelineStageFlags transition_1_stage_flags[1] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	{	// Third submit
		const VkSubmitInfo submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &semaphore_transfer_finished,
			.pWaitDstStageMask = transition_1_stage_flags,
			.commandBufferCount = 1,
			.pCommandBuffers = &transition_1_command_buffer,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = NULL
		};
		vkQueueSubmit(graphics_queue, 1, &submit_info, NULL);
	}

	vkQueueWaitIdle(graphics_queue);
	vkQueueWaitIdle(transfer_queue);

	vkFreeCommandBuffers(device, render_command_pool, 1, &transition_command_buffer);
	vkFreeCommandBuffers(device, render_command_pool, 1, &transition_1_command_buffer);
	vkFreeCommandBuffers(device, transfer_command_pool, 1, &transfer_command_buffer);

	vkDestroySemaphore(device, semaphore_transition_finished, NULL);
	vkDestroySemaphore(device, semaphore_transfer_finished, NULL);

	if (texture_create_info.type == TEXTURE_TYPE_TILEMAP) {
		texture.layout = VK_IMAGE_LAYOUT_GENERAL;
	}
	else {
		texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	return texture;
}
