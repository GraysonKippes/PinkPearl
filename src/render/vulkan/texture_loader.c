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



// TODO - eventually (maybe) collate this with the global staging buffer.
static staging_buffer_t image_staging_buffer = { 
	.handle = VK_NULL_HANDLE,
	.memory = VK_NULL_HANDLE,
	.size = 0,
	.mapped_memory = NULL,
	.device = VK_NULL_HANDLE
};

void create_image_staging_buffer(void) {

	static const VkDeviceSize image_staging_buffer_size = 262144LL;

	image_staging_buffer.device = device;
	image_staging_buffer.size = image_staging_buffer_size;

	VkBufferCreateInfo buffer_create_info = { 0 };
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = NULL;
	buffer_create_info.flags = 0;
	buffer_create_info.size = image_staging_buffer.size;
	buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = NULL;

	VkResult buffer_create_result = vkCreateBuffer(image_staging_buffer.device, &buffer_create_info, NULL, &image_staging_buffer.handle);
	if (buffer_create_result != VK_SUCCESS) {
		logf_message(FATAL, "Image staging buffer for texture loading failed. (Error code: %i)", buffer_create_result);
		return;
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(image_staging_buffer.device, image_staging_buffer.handle, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = memory_type_set.resource_staging;

	// TODO - error handling
	vkAllocateMemory(image_staging_buffer.device, &allocate_info, NULL, &image_staging_buffer.memory);

	vkBindBufferMemory(image_staging_buffer.device, image_staging_buffer.handle, image_staging_buffer.memory, 0);

	vkMapMemory(image_staging_buffer.device, image_staging_buffer.memory, 0, VK_WHOLE_SIZE, 0, &image_staging_buffer.mapped_memory);
}

void destroy_image_staging_buffer(void) {

	vkDestroyBuffer(image_staging_buffer.device, image_staging_buffer.handle, NULL);
	vkFreeMemory(image_staging_buffer.device, image_staging_buffer.memory, NULL);

	image_staging_buffer.handle = VK_NULL_HANDLE;
	image_staging_buffer.memory = VK_NULL_HANDLE;
	image_staging_buffer.size = 0;
	image_staging_buffer.mapped_memory = NULL;
	image_staging_buffer.device = VK_NULL_HANDLE;
}



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

static VkFormat texture_image_format(const texture_type_t texture_type) {
	switch (texture_type) {
		case TEXTURE_TYPE_NORMAL: return VK_FORMAT_R8G8B8A8_SRGB;
		case TEXTURE_TYPE_TILEMAP: return VK_FORMAT_R8G8B8A8_UINT;
	}
	return VK_FORMAT_UNDEFINED;
}

static VkImageUsageFlags texture_image_usage(const texture_type_t texture_type) {
	switch (texture_type) {
		case TEXTURE_TYPE_NORMAL: return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		case TEXTURE_TYPE_TILEMAP: return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	}
	return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
}

static VkImage create_texture_image(const texture_t texture, const texture_create_info_t texture_create_info, const animation_create_info_t animation_create_info) {

	uint32_t queue_family_indices[2];
	if (texture_create_info.type == TEXTURE_TYPE_TILEMAP) {
		queue_family_indices[0] = *physical_device.queue_family_indices.transfer_family_ptr;
		queue_family_indices[1] = *physical_device.queue_family_indices.compute_family_ptr;
	}
	else {
		queue_family_indices[0] = *physical_device.queue_family_indices.graphics_family_ptr;
		queue_family_indices[1] = *physical_device.queue_family_indices.transfer_family_ptr;
	}

	// Create image.
	const VkImageCreateInfo image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = texture.format,
		.extent.width = texture_create_info.cell_extent.width,
		.extent.height = texture_create_info.cell_extent.length,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = animation_create_info.num_frames,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = texture_image_usage(texture_create_info.type),
		.sharingMode = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = 2,
		.pQueueFamilyIndices = queue_family_indices,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkImage vk_image = VK_NULL_HANDLE;
	const VkResult image_create_result = vkCreateImage(texture.device, &image_create_info, NULL, &vk_image);
	if (image_create_result != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: image creation failed. (Error code: %i)", image_create_result);
	}
	return vk_image;
}

static void get_image_memory_requirements(const VkDevice device, const VkImage vk_image, VkMemoryRequirements2 *const memory_requirements) {

	// Query memory requirements for this image.
	VkImageMemoryRequirementsInfo2 image_memory_requirements_info = { 0 };
	image_memory_requirements_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
	image_memory_requirements_info.pNext = NULL;
	image_memory_requirements_info.image = vk_image;

	VkMemoryRequirements2 image_memory_requirements = { 0 };
	image_memory_requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	image_memory_requirements.pNext = NULL;

	vkGetImageMemoryRequirements2(device, &image_memory_requirements_info, &image_memory_requirements);
	*memory_requirements = image_memory_requirements;
}

static void bind_image_memory(const texture_t texture, const VkImage vk_image, VkDeviceSize offset) {

	// Bind the image to memory.
	const VkBindImageMemoryInfo image_bind_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
		.pNext = NULL,
		.image = vk_image,
		.memory = texture.memory,
		.memoryOffset = offset
	};

	vkBindImageMemory2(texture.device, 1, &image_bind_info);
}

static VkImageView create_texture_image_view(const texture_t texture, const VkImage vk_image) {

	// Subresource range used in all image views and layout transitions.
	static const VkImageSubresourceRange image_subresource_range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};

	// Create image view.
	const VkImageViewCreateInfo image_view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = vk_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		.format = texture.format,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange = image_subresource_range
	};

	VkImageView vk_image_view = VK_NULL_HANDLE;
	const VkResult result = vkCreateImageView(texture.device, &image_view_create_info, NULL, &vk_image_view);
	if (result != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: image view creation failed. (Error code: %i)", result);
	}
	return vk_image_view;
}

texture_t load_texture(const texture_create_info_t texture_create_info) {

	// TODO - modify this to use semaphores between image transitions and data transfer operations.

	logf_message(VERBOSE, "Loading texture at \"%s\"...", texture_create_info.path);

	texture_t texture = make_null_texture();

	if (texture_create_info.path == NULL) {
		log_message(ERROR, "Error loading texture: texture path is NULL.");
		return texture;
	}

	if (texture_create_info.num_animations > 0 && texture_create_info.animations == NULL) {
		log_message(ERROR, "Error loading texture: number of animations is greater than zero, but array of animation create infos is NULL.");
		return texture;
	}

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
	image_data_t base_image_data = load_image_data(path, 0);
	const VkDeviceSize base_image_width = base_image_data.width;
	const VkDeviceSize base_image_height = base_image_data.height;
	const VkDeviceSize base_image_channels = base_image_data.num_channels;
	const VkDeviceSize base_image_size = base_image_width * base_image_height * base_image_channels;
	memcpy(image_staging_buffer.mapped_memory, base_image_data.data, base_image_size);
	free_image_data(base_image_data);

	/* TEXTURE INITIAL STATE */

	texture.num_images = texture_create_info.num_animations;
	if (!allocate((void **)&texture.images, texture.num_images, sizeof(texture_image_t))) {
		log_message(ERROR, "Error loading texture: failed to allocate texture image pointer-array.");
		return texture;
	}
	texture.format = texture_image_format(texture_create_info.type);
	texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	texture.memory = VK_NULL_HANDLE;
	texture.device = device;

	VkMemoryRequirements2 *image_memory_requirements = NULL;
	if (!allocate((void **)&image_memory_requirements, texture.num_images, sizeof(VkMemoryRequirements2))) {
		log_message(ERROR, "Error loading texture: failed to allocate memory requirements pointer-array.");
		return texture;
	}

	// Create vulkan images and accumulate the memory requirements.
	VkDeviceSize total_required_memory_size = 0;
	for (uint32_t i = 0; i < texture.num_images; ++i) {

		texture.images[i].vk_image = create_texture_image(texture, texture_create_info, texture_create_info.animations[i]);
		get_image_memory_requirements(texture.device, texture.images[i].vk_image, &image_memory_requirements[i]);
		total_required_memory_size += image_memory_requirements[i].memoryRequirements.size;

		// Copy texture animation cycle from animation set.
		texture.images[i].image_array_length = texture_create_info.animations[i].num_frames;
		texture.images[i].frames_per_second = texture_create_info.animations[i].frames_per_second;
	}

	// Allocate memory for the texture.
	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = total_required_memory_size;
	allocate_info.memoryTypeIndex = memory_type_set.graphics_resources;

	const VkResult memory_allocation_result = vkAllocateMemory(device, &allocate_info, NULL, &texture.memory);
	if (memory_allocation_result != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: failed to allocate memory. (Error code: %i)", memory_allocation_result);
		// TODO - clean up previous image objects and return here.
	}

	// Subresource range used in all image views and layout transitions.
	static const VkImageSubresourceRange image_subresource_range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};

	// Bind each image to memory and create an image view for it.
	VkDeviceSize accumulated_memory_offset = 0;
	for (uint32_t i = 0; i < texture.num_images; ++i) {
		bind_image_memory(texture, texture.images[i].vk_image, accumulated_memory_offset);
		accumulated_memory_offset += image_memory_requirements[i].memoryRequirements.size;
		texture.images[i].vk_image_view = create_texture_image_view(texture, texture.images[i].vk_image);
	}

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

			copy_regions[j].bufferOffset = (VkDeviceSize)texel_offset * bytes_per_texel;
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
		copy_info.srcBuffer = image_staging_buffer.handle;
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
