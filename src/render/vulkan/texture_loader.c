#include "texture_loader.h"

#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "config.h"
#include "log/logging.h"
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

texture_t load_texture(const texture_create_info_t texture_create_info) {

	// TODO - modify this to use semaphores between image transitions and data transfer operations.

	logf_message(VERBOSE, "Loading texture at \"%s\"...", texture_create_info.path);

	if (texture_create_info.path == NULL) {
		log_message(ERROR, "Error loading texture: texture path is NULL.");
		return make_null_texture();
	}

	if (texture_create_info.num_animations > 0 && texture_create_info.animations == NULL) {
		log_message(ERROR, "Error loading texture: number of animations is greater than zero, but array of animation create infos is NULL.");
		return make_null_texture();
	}

	texture_t texture = create_texture(texture_create_info);

	// Create semaphores.
	VkSemaphore semaphore_transition_finished = VK_NULL_HANDLE;
	VkSemaphore semaphore_transfer_finished = VK_NULL_HANDLE;

	VkSemaphoreCreateInfo semaphore_create_info = { 0 };
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_create_info.pNext = NULL;
	semaphore_create_info.flags = 0;

	vkCreateSemaphore(device, &semaphore_create_info, NULL, &semaphore_transition_finished);
	vkCreateSemaphore(device, &semaphore_create_info, NULL, &semaphore_transfer_finished);

	// Create full path.
	char path[TEXTURE_NAME_MAX_LENGTH];
	make_image_path(texture_create_info.path, path);

	// Load image data into image staging buffer.
	byte_t *const mapped_memory = buffer_partition_map_memory(global_staging_buffer_partition, 1);
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
	begin_command_buffer(transition_command_buffer, 0);

	// Transition each image's layout from undefined to transfer destination.
	for (uint32_t i = 0; i < texture.num_images; ++i) {
		
		VkImageMemoryBarrier image_memory_barrier = { 0 };
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.pNext = NULL;
		image_memory_barrier.srcAccessMask = VK_ACCESS_NONE;
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image = texture.images[i].vk_image;
		image_memory_barrier.subresourceRange = image_subresource_range;

		VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		vkCmdPipelineBarrier(transition_command_buffer, source_stage, destination_stage, 0,
				0, NULL,
				0, NULL,
				1, &image_memory_barrier);
	}

	vkEndCommandBuffer(transition_command_buffer);

	VkSubmitInfo transition_0_submit_info = { 0 };
	transition_0_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	transition_0_submit_info.pNext = NULL;
	transition_0_submit_info.waitSemaphoreCount = 0;
	transition_0_submit_info.pWaitSemaphores = NULL;
	transition_0_submit_info.pWaitDstStageMask = NULL;
	transition_0_submit_info.commandBufferCount = 1;
	transition_0_submit_info.pCommandBuffers = &transition_command_buffer;
	transition_0_submit_info.signalSemaphoreCount = 1;
	transition_0_submit_info.pSignalSemaphores = &semaphore_transition_finished;

	vkQueueSubmit(graphics_queue, 1, &transition_0_submit_info, NULL);

	texture.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	// Transfer image data to texture images.
	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, transfer_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	for (uint32_t i = 0; i < texture.num_images; ++i) {
		
		const uint32_t num_copy_regions = texture.images[i].image_array_length;
		VkBufferImageCopy2 *copy_regions = NULL;

		if (!allocate((void **)&copy_regions, num_copy_regions, sizeof(VkBufferImageCopy2))) {
			log_message(ERROR, "Error loading texture: failed to allocate copy region pointer-array.");
			// TODO - do proper cleanup here.
			return texture;
		}

		const VkDeviceSize buffer_partition_offset = global_staging_buffer_partition.ranges[1].offset;
		for (uint32_t j = 0; j < num_copy_regions; ++j) {

			copy_regions[j].sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
			copy_regions[j].pNext = NULL;

			uint32_t cell_offset = texture_create_info.animations[i].start_cell + j;
			uint32_t cell_offset_x = cell_offset % texture_create_info.num_cells.width;
			uint32_t cell_offset_y = cell_offset / texture_create_info.num_cells.width;

			uint32_t texel_offset_x = cell_offset_x * texture_create_info.cell_extent.width;
			uint32_t texel_offset_y = cell_offset_y * texture_create_info.cell_extent.length;
			uint32_t texel_offset = texel_offset_y * texture_create_info.atlas_extent.width + texel_offset_x;

			static const VkDeviceSize bytes_per_texel = 4;

			copy_regions[j].bufferOffset = buffer_partition_offset + (VkDeviceSize)texel_offset * bytes_per_texel;
			copy_regions[j].bufferRowLength = texture_create_info.atlas_extent.width;
			copy_regions[j].bufferImageHeight = texture_create_info.atlas_extent.length;

			copy_regions[j].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_regions[j].imageSubresource.mipLevel = 0;
			copy_regions[j].imageSubresource.baseArrayLayer = j;
			copy_regions[j].imageSubresource.layerCount = 1;

			copy_regions[j].imageOffset.x = 0;
			copy_regions[j].imageOffset.y = 0;
			copy_regions[j].imageOffset.z = 0;

			copy_regions[j].imageExtent.width = texture_create_info.cell_extent.width;
			copy_regions[j].imageExtent.height = texture_create_info.cell_extent.length;
			copy_regions[j].imageExtent.depth = 1;
		}

		VkCopyBufferToImageInfo2 copy_info = { 0 };
		copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
		copy_info.pNext = NULL;
		copy_info.srcBuffer = global_staging_buffer_partition.buffer;
		copy_info.dstImage = texture.images[i].vk_image;
		copy_info.dstImageLayout = texture.layout;
		copy_info.regionCount = num_copy_regions;
		copy_info.pRegions = copy_regions;

		vkCmdCopyBufferToImage2(transfer_command_buffer, &copy_info);

		deallocate((void **)&copy_regions);
	}

	vkEndCommandBuffer(transfer_command_buffer);

	VkPipelineStageFlags transfer_stage_flags[1] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	VkSubmitInfo transfer_submit_info = { 0 };
	transfer_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	transfer_submit_info.pNext = NULL;
	transfer_submit_info.waitSemaphoreCount = 1;
	transfer_submit_info.pWaitSemaphores = &semaphore_transition_finished;
	transfer_submit_info.pWaitDstStageMask = transfer_stage_flags;
	transfer_submit_info.commandBufferCount = 1;
	transfer_submit_info.pCommandBuffers = &transfer_command_buffer;
	transfer_submit_info.signalSemaphoreCount = 1;
	transfer_submit_info.pSignalSemaphores = &semaphore_transfer_finished;

	vkQueueSubmit(transfer_queue, 1, &transfer_submit_info, NULL);

	// Command buffer for second image layout transition (transfer destination to sampled).
	VkCommandBuffer transition_1_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transition_1_command_buffer);
	begin_command_buffer(transition_1_command_buffer, 0);

	// Transition each image's layout from transfer destination to sampled.
	for (uint32_t i = 0; i < texture.num_images; ++i) {
		
		VkImageMemoryBarrier image_memory_barrier = { 0 };
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.pNext = NULL;
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image = texture.images[i].vk_image;
		image_memory_barrier.subresourceRange = image_subresource_range;

		VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
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
	}

	vkEndCommandBuffer(transition_1_command_buffer);

	VkPipelineStageFlags transition_1_stage_flags[1] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	VkSubmitInfo transition_1_submit_info = { 0 };
	transition_1_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	transition_1_submit_info.pNext = NULL;
	transition_1_submit_info.waitSemaphoreCount = 1;
	transition_1_submit_info.pWaitSemaphores = &semaphore_transfer_finished;
	transition_1_submit_info.pWaitDstStageMask = transition_1_stage_flags;
	transition_1_submit_info.commandBufferCount = 1;
	transition_1_submit_info.pCommandBuffers = &transition_1_command_buffer;
	transition_1_submit_info.signalSemaphoreCount = 0;
	transition_1_submit_info.pSignalSemaphores = NULL;

	vkQueueSubmit(graphics_queue, 1, &transition_1_submit_info, NULL);

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
