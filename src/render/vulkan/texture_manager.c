#include "texture_manager.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "log/logging.h"
#include "render/stb/image_data.h"

#include "buffer.h"
#include "command_buffer.h"
#include "texture.h"
#include "vulkan_manager.h"

#define NUM_TEXTURES 1

static texture_t textures[NUM_TEXTURES];

#define IMAGE_STAGING_BUFFER_SIZE 65536

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

void load_texture(animation_set_t animation_set, const char *path) {
	
	static uint32_t num_textures_loaded = 0;

	logf_message(VERBOSE, "Loading texture %u...", num_textures_loaded);

	if (num_textures_loaded >= NUM_TEXTURES) {
		log_message(ERROR, "Error loading texture: attempted loading an additional texture past max number of textures.");
		return;
	}

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
	textures[num_textures_loaded].num_images = animation_set.num_animations;
	textures[num_textures_loaded].images = calloc(animation_set.num_animations, sizeof(VkImage));
	textures[num_textures_loaded].image_views = calloc(animation_set.num_animations, sizeof(VkImageView));
	textures[num_textures_loaded].animation_cycles = calloc(animation_set.num_animations, sizeof(texture_animation_cycle_t));
	textures[num_textures_loaded].format = image_format_default;
	textures[num_textures_loaded].layout = VK_IMAGE_LAYOUT_UNDEFINED;
	textures[num_textures_loaded].memory = VK_NULL_HANDLE;
	textures[num_textures_loaded].device = device;

	// Memory requirments of each image.
	VkMemoryRequirements2 *image_memory_requirements = calloc(animation_set.num_animations, sizeof(VkMemoryRequirements2));
	VkDeviceSize total_required_memory_size = 0;

	// Create images and animation cycles.
	for (uint32_t i = 0; i < animation_set.num_animations; ++i) {

		// Create image.
		VkImageCreateInfo image_create_info = { 0 };
		image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_create_info.pNext = NULL;
		image_create_info.flags = 0;
		image_create_info.imageType = image_type_default;
		image_create_info.format = textures[num_textures_loaded].format;
		image_create_info.extent.width = animation_set.cell_extent.width;
		image_create_info.extent.height = animation_set.cell_extent.length;
		image_create_info.extent.depth = 1;
		image_create_info.mipLevels = 1;
		image_create_info.arrayLayers = animation_set.animations[i].num_frames;
		image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkResult image_create_result = vkCreateImage(textures[num_textures_loaded].device, &image_create_info, NULL, (textures[num_textures_loaded].images + i));
		if (image_create_result != VK_SUCCESS) {
			logf_message(ERROR, "Error loading texture: image creation failed. (Error code: %i)", image_create_result);
			return;
		}

		// Query memory requirements for this image.
		VkImageMemoryRequirementsInfo2 image_memory_requirements_info = { 0 };
		image_memory_requirements_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
		image_memory_requirements_info.pNext = NULL;
		image_memory_requirements_info.image = textures[num_textures_loaded].images[i];

		image_memory_requirements[i].sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		image_memory_requirements[i].pNext = NULL;

		vkGetImageMemoryRequirements2(textures[num_textures_loaded].device, &image_memory_requirements_info, (image_memory_requirements + i));
		total_required_memory_size += image_memory_requirements[i].memoryRequirements.size;

		// Copy texture animation cycle from animation set.
		textures[num_textures_loaded].animation_cycles[i] = (texture_animation_cycle_t){
			.current_frame = 0,
			.play_rate = 1,
			.num_frames = animation_set.animations[i].num_frames,
			.frames_per_second = animation_set.animations[i].frames_per_second
		};
	}

	// Allocate memory for the texture.
	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = total_required_memory_size;
	allocate_info.memoryTypeIndex = memory_type_set.graphics_resources;

	VkResult memory_allocation_result = vkAllocateMemory(device, &allocate_info, NULL, &textures[num_textures_loaded].memory);
	if (memory_allocation_result != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: failed to allocate memory. (Error code: %i)", memory_allocation_result);
		// TODO - clean up previous image objects and return here.
	}

	// Subresource range used in all image views and layout transitions.
	const VkImageSubresourceRange image_subresource_range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};

	// Bind each image to memory and create a view for it.
	VkDeviceSize accumulated_memory_offset = 0;
	for (uint32_t i = 0; i < animation_set.num_animations; ++i) {

		// Bind the image to memory.
		VkBindImageMemoryInfo image_bind_info = { 0 };
		image_bind_info.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
		image_bind_info.pNext = NULL;
		image_bind_info.image = textures[num_textures_loaded].images[i];
		image_bind_info.memory = textures[num_textures_loaded].memory;
		image_bind_info.memoryOffset = accumulated_memory_offset;

		vkBindImageMemory2(device, 1, &image_bind_info);

		accumulated_memory_offset += image_memory_requirements[i].memoryRequirements.size;

		// Create image view.
		VkImageViewCreateInfo image_view_create_info = { 0 };
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.pNext = NULL;
		image_view_create_info.flags = 0;
		image_view_create_info.image = textures[num_textures_loaded].images[i];
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		image_view_create_info.format = textures[num_textures_loaded].format;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.subresourceRange = image_subresource_range;

		VkResult image_view_create_result = vkCreateImageView(textures[num_textures_loaded].device, &image_view_create_info, NULL, (textures[num_textures_loaded].image_views + i));
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
	for (uint32_t i = 0; i < animation_set.num_animations; ++i) {
		
		VkImageMemoryBarrier image_memory_barrier = { 0 };
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.pNext = NULL;
		image_memory_barrier.srcAccessMask = VK_ACCESS_NONE;
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image = textures[num_textures_loaded].images[i];
		image_memory_barrier.subresourceRange = image_subresource_range;

		VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		vkCmdPipelineBarrier(transition_command_buffer, source_stage, destination_stage, 0,
				0, NULL,
				0, NULL,
				1, &image_memory_barrier);
	}

	vkEndCommandBuffer(transition_command_buffer);
	submit_command_buffers_async(graphics_queue, 1, &transition_command_buffer);
	textures[num_textures_loaded].layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	vkResetCommandBuffer(transition_command_buffer, 0);

	// Transfer image data to texture images.
	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, transfer_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	for (uint32_t i = 0; i < animation_set.num_animations; ++i) {
		
		const uint32_t num_copy_regions = animation_set.animations[i].num_frames;
		VkBufferImageCopy2 *copy_regions = calloc(num_copy_regions, sizeof(VkBufferImageCopy2));

		for (uint32_t j = 0; j < num_copy_regions; ++j) {

			copy_regions[j].sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
			copy_regions[j].pNext = NULL;

			uint32_t texel_offset_x = j * animation_set.cell_extent.width;
			uint32_t texel_offset_y = i * animation_set.atlas_extent.width * animation_set.cell_extent.length;

			copy_regions[j].bufferOffset = (VkDeviceSize)(texel_offset_x + texel_offset_y) * 4;
			copy_regions[j].bufferRowLength = animation_set.atlas_extent.width;
			copy_regions[j].bufferImageHeight = animation_set.atlas_extent.length;

			copy_regions[j].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_regions[j].imageSubresource.mipLevel = 0;
			copy_regions[j].imageSubresource.baseArrayLayer = j;
			copy_regions[j].imageSubresource.layerCount = 1;

			copy_regions[j].imageOffset.x = 0;
			copy_regions[j].imageOffset.y = 0;
			copy_regions[j].imageOffset.z = 0;

			copy_regions[j].imageExtent.width = animation_set.cell_extent.width;
			copy_regions[j].imageExtent.height = animation_set.cell_extent.length;
			copy_regions[j].imageExtent.depth = 1;
		}

		VkCopyBufferToImageInfo2 copy_info = { 0 };
		copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
		copy_info.pNext = NULL;
		copy_info.srcBuffer = image_staging_buffer.handle;
		copy_info.dstImage = textures[num_textures_loaded].images[i];
		copy_info.dstImageLayout = textures[num_textures_loaded].layout;
		copy_info.regionCount = num_copy_regions;
		copy_info.pRegions = copy_regions;

		vkCmdCopyBufferToImage2(transfer_command_buffer, &copy_info);

		free(copy_regions);
	}

	vkEndCommandBuffer(transfer_command_buffer);
	submit_command_buffers_async(transfer_queue, 1, &transfer_command_buffer);
	vkFreeCommandBuffers(device, transfer_command_pool, 1, &transfer_command_buffer);

	// Command buffer for second image layout transition (transfer destination to sampled).
	begin_command_buffer(transition_command_buffer, 0);

	// Transition each image's layout from transfer destination to sampled.
	for (uint32_t i = 0; i < animation_set.num_animations; ++i) {
		
		VkImageMemoryBarrier image_memory_barrier = { 0 };
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.pNext = NULL;
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image = textures[num_textures_loaded].images[i];
		image_memory_barrier.subresourceRange = image_subresource_range;

		VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		vkCmdPipelineBarrier(transition_command_buffer, source_stage, destination_stage, 0,
				0, NULL,
				0, NULL,
				1, &image_memory_barrier);
	}

	vkEndCommandBuffer(transition_command_buffer);
	submit_command_buffers_async(graphics_queue, 1, &transition_command_buffer);
	textures[num_textures_loaded].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkFreeCommandBuffers(device, render_command_pool, 1, &transition_command_buffer);
	
	num_textures_loaded++;
}

void load_textures(void) {
	
	create_image_staging_buffer();

	animation_set_t animation_set = {
		.atlas_extent = (extent_t){ 48, 96 },
		.num_cells = (extent_t){ 3, 4 },
		.cell_extent = (extent_t){ 16, 24 },
		.num_animations = 4,
		.animations = (animation_t[4]){
			(animation_t){
				.start_cell = 0,
				.num_frames = 3,
				.frames_per_second = 10
			},
			(animation_t){
				.start_cell = 3,
				.num_frames = 3,
				.frames_per_second = 10
			},
			(animation_t){
				.start_cell = 6,
				.num_frames = 3,
				.frames_per_second = 10
			},
			(animation_t){
				.start_cell = 9,
				.num_frames = 3,
				.frames_per_second = 10
			}
		}
	};

	load_texture(animation_set, "../resources/assets/textures/entity/pearl.png");

	destroy_image_staging_buffer();
}

void destroy_texture(texture_t texture) {

	for (uint32_t i = 0; i < texture.num_images; ++i) {
		vkDestroyImage(texture.device, texture.images[i], NULL);
		vkDestroyImageView(texture.device, texture.image_views[i], NULL);
	}

	vkFreeMemory(device, texture.memory, NULL);

	free(texture.images);
	free(texture.image_views);
	free(texture.animation_cycles);
}

void destroy_textures(void) {
	for (uint32_t i = 0; i < NUM_TEXTURES; ++i) {
		destroy_texture(textures[i]);
	}
}
