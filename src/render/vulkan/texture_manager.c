#include "texture_manager.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>

#include <vulkan/vulkan.h>

#include "config.h"
#include "log/logging.h"
#include "render/stb/image_data.h"

#include "buffer.h"
#include "command_buffer.h"
#include "texture.h"
#include "vulkan_manager.h"



#define TEXTURE_PATH (RESOURCE_PATH "assets/textures/")

#define NUM_TEXTURES 3

static const uint32_t num_textures = NUM_TEXTURES;

#define TEXTURE_NAME_MAX_LENGTH 256

static const size_t texture_name_max_length = TEXTURE_NAME_MAX_LENGTH;

static texture_t textures[NUM_TEXTURES];

static char texture_names[NUM_TEXTURES][TEXTURE_NAME_MAX_LENGTH];

#define IMAGE_STAGING_BUFFER_SIZE 65536



texture_create_info_t make_texture_create_info(extent_t texture_extent) {

	static const animation_create_info_t animations[1] = {
		{
			.start_cell = 0,
			.num_frames = 1,
			.frames_per_second = 0
		}
	};

	texture_create_info_t texture_create_info = { 0 };
	texture_create_info.atlas_extent = texture_extent;
	texture_create_info.num_cells = (extent_t){ 1, 1 };
	texture_create_info.cell_extent = texture_extent;
	texture_create_info.num_animations = 1;
	texture_create_info.animations = (animation_create_info_t *)animations;

	return texture_create_info;
}



// TODO - eventually (maybe) collate this with the global staging buffer.
static staging_buffer_t image_staging_buffer = { 
	.handle = VK_NULL_HANDLE,
	.memory = VK_NULL_HANDLE,
	.size = IMAGE_STAGING_BUFFER_SIZE,
	.mapped_memory = NULL,
	.device = VK_NULL_HANDLE
};

void create_image_staging_buffer(void) {

	image_staging_buffer.device = device;

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

void load_texture(const texture_create_info_t texture_create_info, const bool is_tilemap) {

	// TODO - modify this to use semaphores between image transitions and data transfer operations.

	static uint32_t num_textures_loaded = 0;

	logf_message(VERBOSE, "Loading texture %u...", num_textures_loaded);

	if (num_textures_loaded >= num_textures) {
		log_message(ERROR, "Error loading texture: attempted loading an additional texture past max number of textures.");
		return;
	}

	if (texture_create_info.path == NULL) {
		log_message(ERROR, "Error loading texture: texture path is NULL.");
		return;
	}

	if (texture_create_info.num_animations > 0 && texture_create_info.animations == NULL) {
		log_message(ERROR, "Error loading texture: number of animations is greater than zero, but array of animation create infos is NULL.");
		return;
	}

	const uint32_t texture_index = num_textures_loaded++;

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
	memset(path, '\0', texture_name_max_length);

	const size_t directory_length = strlen(TEXTURE_PATH);
	const errno_t directory_strncpy_result = strncpy_s(path, 256, TEXTURE_PATH, directory_length);
	if (directory_strncpy_result != 0) {
		logf_message(WARNING, "Warning loading texture: failed to copy directory into image path buffer. (Error code: %u)", directory_strncpy_result);
	}

	const size_t filename_length = strnlen_s(texture_create_info.path, 64);
	const errno_t filename_strncat_result = strncat_s(path, 256, texture_create_info.path, filename_length);
	if (filename_strncat_result != 0) {
		logf_message(WARNING, "Warning loading texture: failed to concatenate image filename into image path buffer. (Error code: %u)", filename_strncat_result);
	}

	logf_message(VERBOSE, "Loading image at \"%s\"...", path);
	logf_message(VERBOSE, "Texture is tilemap: %i", is_tilemap);

	// Copy texture name into texture name array.

	// Temporary name buffer -- do the string operations here.
	char name[TEXTURE_NAME_MAX_LENGTH];
	memset(name, '\0', texture_name_max_length);
	const errno_t texture_name_strncpy_result_1 = strncpy_s(name, texture_name_max_length, texture_create_info.path, 64);
	if (texture_name_strncpy_result_1 != 0) {
		logf_message(WARNING, "Warning loading texture: failed to copy texture path into temporary texture name buffer. (Error code: %u)", texture_name_strncpy_result_1);
	}

	// Prune off the file extension if it is found.
	// TODO - make a safer version of this function...
	char *file_extension_delimiter_ptr = strrchr(name, '.');
	if (file_extension_delimiter_ptr != NULL) {
		const ptrdiff_t file_extension_delimiter_offset = file_extension_delimiter_ptr - name;
		const size_t remaining_length = file_extension_delimiter_offset >= 0 
			? texture_name_max_length - file_extension_delimiter_offset
			: texture_name_max_length;
		memset(file_extension_delimiter_ptr, '\0', remaining_length);
	}

	// Copy the processed texture name into the array of texture names.
	const errno_t texture_name_strncpy_result_2 = strncpy_s(texture_names[texture_index], texture_name_max_length, name, texture_name_max_length);
	if (texture_name_strncpy_result_2 != 0) {
		logf_message(WARNING, "Warning loading texture: failed to copy texture name from temporary buffer into texture name array. (Error code: %u)", texture_name_strncpy_result_2);
	}
	
	logf_message(VERBOSE, "Texture's recorded name is \"%s\".", texture_names[texture_index]);

	// Load image data into image staging buffer.
	image_data_t base_image_data = load_image_data(path, 0);
	const VkDeviceSize base_image_width = base_image_data.width;
	const VkDeviceSize base_image_height = base_image_data.height;
	const VkDeviceSize base_image_channels = base_image_data.num_channels;
	const VkDeviceSize base_image_size = base_image_width * base_image_height * base_image_channels;
	memcpy(image_staging_buffer.mapped_memory, base_image_data.data, base_image_size);
	free_image_data(base_image_data);

	// Create texture images.
	static const VkImageType image_type_default = VK_IMAGE_TYPE_2D;
	static const VkFormat image_format_default = VK_FORMAT_R8G8B8A8_SRGB;

	// Initial state.
	textures[texture_index].num_images = texture_create_info.num_animations;
	textures[texture_index].images = calloc(texture_create_info.num_animations, sizeof(VkImage));
	textures[texture_index].image_views = calloc(texture_create_info.num_animations, sizeof(VkImageView));
	textures[texture_index].animation_cycles = calloc(texture_create_info.num_animations, sizeof(texture_animation_cycle_t));
	textures[texture_index].current_animation_cycle = 0;
	textures[texture_index].current_animation_frame = 0;
	textures[texture_index].last_frame_time = 0;
	textures[texture_index].format = image_format_default;
	textures[texture_index].layout = VK_IMAGE_LAYOUT_UNDEFINED;
	textures[texture_index].memory = VK_NULL_HANDLE;
	textures[texture_index].device = device;

	if (is_tilemap) {
		textures[texture_index].format = VK_FORMAT_R8G8B8A8_UINT;
	}
	else {
		textures[texture_index].format = image_format_default;
	}

	// Memory requirments of each image.
	VkMemoryRequirements2 *image_memory_requirements = calloc(texture_create_info.num_animations, sizeof(VkMemoryRequirements2));
	VkDeviceSize total_required_memory_size = 0;

	// Create images and animation cycles.
	for (uint32_t i = 0; i < textures[texture_index].num_images; ++i) {

		// Create image.
		VkImageCreateInfo image_create_info = { 0 };
		image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_create_info.pNext = NULL;
		image_create_info.flags = 0;
		image_create_info.imageType = image_type_default;
		image_create_info.format = textures[texture_index].format;
		image_create_info.extent.width = texture_create_info.cell_extent.width;
		image_create_info.extent.height = texture_create_info.cell_extent.length;
		image_create_info.extent.depth = 1;
		image_create_info.mipLevels = 1;
		image_create_info.arrayLayers = texture_create_info.animations[i].num_frames;
		image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (is_tilemap) {

			uint32_t queue_family_indices[2] = {
					*physical_device.queue_family_indices.transfer_family_ptr,
					*physical_device.queue_family_indices.compute_family_ptr
			};

			image_create_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
			image_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
			image_create_info.queueFamilyIndexCount = 2;
			image_create_info.pQueueFamilyIndices = queue_family_indices;
		}
		else {

			uint32_t queue_family_indices[2] = {
					*physical_device.queue_family_indices.graphics_family_ptr,
					*physical_device.queue_family_indices.transfer_family_ptr
			};

			image_create_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			image_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
			image_create_info.queueFamilyIndexCount = 2;
			image_create_info.pQueueFamilyIndices = queue_family_indices;
		}

		VkResult image_create_result = vkCreateImage(textures[texture_index].device, &image_create_info, NULL, (textures[texture_index].images + i));
		if (image_create_result != VK_SUCCESS) {
			logf_message(ERROR, "Error loading texture: image creation failed. (Error code: %i)", image_create_result);
			return;
		}

		// Query memory requirements for this image.
		VkImageMemoryRequirementsInfo2 image_memory_requirements_info = { 0 };
		image_memory_requirements_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
		image_memory_requirements_info.pNext = NULL;
		image_memory_requirements_info.image = textures[texture_index].images[i];

		image_memory_requirements[i].sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		image_memory_requirements[i].pNext = NULL;

		vkGetImageMemoryRequirements2(textures[texture_index].device, &image_memory_requirements_info, (image_memory_requirements + i));
		total_required_memory_size += image_memory_requirements[i].memoryRequirements.size;

		// Copy texture animation cycle from animation set.
		textures[texture_index].animation_cycles[i] = (texture_animation_cycle_t){
			.num_frames = texture_create_info.animations[i].num_frames,
			.frames_per_second = texture_create_info.animations[i].frames_per_second
		};
	}

	// Allocate memory for the texture.
	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = total_required_memory_size;
	allocate_info.memoryTypeIndex = memory_type_set.graphics_resources;

	VkResult memory_allocation_result = vkAllocateMemory(device, &allocate_info, NULL, &textures[texture_index].memory);
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

	// Bind each image to memory and create a view for it.
	VkDeviceSize accumulated_memory_offset = 0;
	for (uint32_t i = 0; i < textures[texture_index].num_images; ++i) {

		// Bind the image to memory.
		VkBindImageMemoryInfo image_bind_info = { 0 };
		image_bind_info.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
		image_bind_info.pNext = NULL;
		image_bind_info.image = textures[texture_index].images[i];
		image_bind_info.memory = textures[texture_index].memory;
		image_bind_info.memoryOffset = accumulated_memory_offset;

		vkBindImageMemory2(device, 1, &image_bind_info);

		accumulated_memory_offset += image_memory_requirements[i].memoryRequirements.size;

		// Create image view.
		VkImageViewCreateInfo image_view_create_info = { 0 };
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.pNext = NULL;
		image_view_create_info.flags = 0;
		image_view_create_info.image = textures[texture_index].images[i];
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		image_view_create_info.format = textures[texture_index].format;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.subresourceRange = image_subresource_range;

		VkResult image_view_create_result = vkCreateImageView(textures[texture_index].device, &image_view_create_info, NULL, (textures[texture_index].image_views + i));
		if (image_view_create_result != VK_SUCCESS) {
			logf_message(ERROR, "Error loading texture: image view creation failed. (Error code: %i)", image_view_create_result);
			return;
		}
	}

	// Command buffer for first image layout transition (undefined to transfer destination).
	VkCommandBuffer transition_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transition_command_buffer);
	begin_command_buffer(transition_command_buffer, 0);

	// Transition each image's layout from undefined to transfer destination.
	for (uint32_t i = 0; i < textures[texture_index].num_images; ++i) {
		
		VkImageMemoryBarrier image_memory_barrier = { 0 };
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.pNext = NULL;
		image_memory_barrier.srcAccessMask = VK_ACCESS_NONE;
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image = textures[texture_index].images[i];
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

	textures[texture_index].layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	// Transfer image data to texture images.
	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, transfer_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	for (uint32_t i = 0; i < textures[texture_index].num_images; ++i) {
		
		const uint32_t num_copy_regions = texture_create_info.animations[i].num_frames;
		VkBufferImageCopy2 *copy_regions = calloc(num_copy_regions, sizeof(VkBufferImageCopy2));

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
		copy_info.dstImage = textures[texture_index].images[i];
		copy_info.dstImageLayout = textures[texture_index].layout;
		copy_info.regionCount = num_copy_regions;
		copy_info.pRegions = copy_regions;

		vkCmdCopyBufferToImage2(transfer_command_buffer, &copy_info);

		free(copy_regions);
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
	for (uint32_t i = 0; i < textures[texture_index].num_images; ++i) {
		
		VkImageMemoryBarrier image_memory_barrier = { 0 };
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.pNext = NULL;
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image = textures[texture_index].images[i];
		image_memory_barrier.subresourceRange = image_subresource_range;

		VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		// If this is a tilemap texture, transition it to the general layout instead 
		// 	so that it can be read by the room texture compute shader.
		if (is_tilemap) {
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

	if (is_tilemap) {
		textures[texture_index].layout = VK_IMAGE_LAYOUT_GENERAL;
	}
	else {
		textures[texture_index].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
}

void load_textures(const texture_pack_t texture_pack) {

	log_message(VERBOSE, "Loading textures...");

	if (texture_pack.num_textures == 0) {
		log_message(WARNING, "Warning loading textures: loaded texture pack is empty.");
	}

	if (texture_pack.texture_create_infos == NULL) {
		log_message(ERROR, "Error loading textures: array of texture create infos is NULL.");
		return;
	}
	
	create_image_staging_buffer();

	texture_create_info_t missing_texture_create_info = { 0 };
	missing_texture_create_info.path = "missing.png";
	missing_texture_create_info.atlas_extent.width = 16;
	missing_texture_create_info.atlas_extent.length = 16;
	missing_texture_create_info.num_cells.width = 1;
	missing_texture_create_info.num_cells.length = 1;
	missing_texture_create_info.cell_extent.width = 16;
	missing_texture_create_info.cell_extent.length = 16;
	missing_texture_create_info.num_animations = 1;
	missing_texture_create_info.animations = (animation_create_info_t[1]){
		{
			.start_cell = 0,
			.num_frames = 1,
			.frames_per_second = 0
		}
	};

	load_texture(missing_texture_create_info, false);

	for (uint32_t i = 0; i < texture_pack.num_textures; ++i) {
		
		logf_message(WARNING, "Texture atlas extent: %u x %u texels.", texture_pack.texture_create_infos[i].atlas_extent.width, texture_pack.texture_create_infos[i].atlas_extent.length);
		logf_message(WARNING, "Texture cell extent: %u x %u texels.", texture_pack.texture_create_infos[i].cell_extent.width, texture_pack.texture_create_infos[i].cell_extent.length);
		logf_message(WARNING, "Texture num. cells: %u x %u cells.", texture_pack.texture_create_infos[i].num_cells.width, texture_pack.texture_create_infos[i].num_cells.length);

		// TODO - this is temporary, make it so that tilemap textures do not need a special flag
		if (i == 0)
			load_texture(texture_pack.texture_create_infos[i], true);
		else
			load_texture(texture_pack.texture_create_infos[i], false);
	}

	destroy_image_staging_buffer();

	log_message(VERBOSE, "Done loading textures.");
}

void destroy_textures(void) {

	log_message(VERBOSE, "Destroying loaded textures...");

	for (uint32_t i = 0; i < NUM_TEXTURES; ++i) {
		destroy_texture(textures[i]);
	}
}

texture_t get_loaded_texture(uint32_t texture_index) {
	
	if (texture_index >= num_textures) {
		logf_message(ERROR, "Error getting loaded texture: texture index (%u) exceeds total number of loaded textures (%u).", texture_index, num_textures);
		return textures[0];
	}

	return textures[texture_index];
}

texture_t find_loaded_texture(const char *const name) {

	if (name == NULL) {
		log_message(ERROR, "Error finding loaded texture: given texture name is NULL.");
		return textures[0];
	}

	const size_t name_length = strnlen_s(name, texture_name_max_length);

	uint32_t index = 0;

	// Depth-first search for matching texture name.

	for (uint32_t i = 0; i < num_textures; ++i) {
	
		bool name_matches = true;

		for (size_t j = 0; j < name_length; ++j) {
			
			const char c = name[j];
			const char d = texture_names[i][j];

			if (c != d) {
				name_matches = false;
				break;
			}
		}

		if (name_matches) {
			index = i;
			break;
		}
	}

	return textures[index];
}
