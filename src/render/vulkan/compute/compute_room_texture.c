#include "compute_room_texture.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "log/logging.h"
#include "render/render_config.h"
#include "util/allocate.h"

#include "../command_buffer.h"
#include "../compute_pipeline.h"
#include "../descriptor.h"
#include "../image.h"
#include "../texture_manager.h"
#include "../vulkan_manager.h"



static const descriptor_binding_t compute_room_texture_bindings[3] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT }
};

static const descriptor_layout_t compute_room_texture_layout = {
	.num_bindings = 3,
	.bindings = (descriptor_binding_t *)compute_room_texture_bindings
};

static compute_pipeline_t compute_room_texture_pipeline;
static image_t computeRoomTextureTransferImage;

// Subresource range used in all image views and layout transitions.
static const VkImageSubresourceRange image_subresource_range = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseMipLevel = 0,
	.levelCount = 1,
	.baseArrayLayer = 0,
	.layerCount = VK_REMAINING_ARRAY_LAYERS
};



Texture createRoomTexture(const room_size_t roomSize) {
	log_message(VERBOSE, "Creating room texture...");

	// Room texture create info only needs the animation parameters.
	// Each "animation" represents a separate texture for a distinct room.
	TextureCreateInfo roomTextureCreateInfo = {
		.textureID = makeNullString(),
		.isLoaded = false,
		.isTilemap = false,
		.atlas_extent = (extent_t){ 0, 0 },
		.num_cells = (extent_t){ num_room_texture_cache_slots, num_room_layers },
		.cell_extent = room_size_to_extent(roomSize),
		.num_animations = num_room_texture_cache_slots,
		.animations = (animation_create_info_t[NUM_ROOM_TEXTURE_CACHE_SLOTS]){ 
			{ 
				.start_cell = 0,
				.num_frames = 0,
				.frames_per_second = 0
			} 
		}
	};

	roomTextureCreateInfo.cell_extent.width *= tile_texel_length;
	roomTextureCreateInfo.cell_extent.length *= tile_texel_length;

	// Populate the room texture create info with cell extents for each room size.
	for (uint32_t i = 0; i < roomTextureCreateInfo.num_animations; ++i) {
		roomTextureCreateInfo.animations[i].start_cell = i;
		roomTextureCreateInfo.animations[i].num_frames = 1;
		roomTextureCreateInfo.animations[i].frames_per_second = 0;
	}

	Texture room_texture = createTexture(roomTextureCreateInfo);
	if (textureIsNull(room_texture)) {
		log_message(ERROR, "Error creating room texture: texture creation failed.");
		return room_texture;
	}

	// Transition the room texture image layout from undefined to shader read-only optimal.

	VkCommandBuffer transition_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(room_texture.device, render_command_pool, 1, &transition_command_buffer);

	const VkCommandBufferBeginInfo transition_command_buffer_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};

	vkBeginCommandBuffer(transition_command_buffer, &transition_command_buffer_begin_info);

	const VkImageMemoryBarrier2 image_memory_barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.pNext = NULL,
		.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		.srcAccessMask = VK_ACCESS_2_NONE,
		.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
		.oldLayout = room_texture.layout,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = room_texture.image.vkImage,
		.subresourceRange = image_subresource_range
	};

	const VkDependencyInfo dependency_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.pNext = NULL,
		.dependencyFlags = 0,
		.memoryBarrierCount = 0,
		.pMemoryBarriers = NULL,
		.bufferMemoryBarrierCount = 0,
		.pBufferMemoryBarriers = NULL,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &image_memory_barrier
	};

	vkCmdPipelineBarrier2(transition_command_buffer, &dependency_info);

	vkEndCommandBuffer(transition_command_buffer);

	const VkSubmitInfo transition_submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = NULL,
		.commandBufferCount = 1,
		.pCommandBuffers = &transition_command_buffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL
	};
	vkQueueSubmit(graphics_queue, 1, &transition_submit_info, NULL);
	vkQueueWaitIdle(graphics_queue);

	room_texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	return room_texture;
}

static void createRoomTextureTransferImage(void) {
	log_message(VERBOSE, "Creating room texture transfer image...");

	// Dimensions for the largest possible room texture, smaller textures use a subsection of this image.
	const extent_t largest_room_extent = room_size_to_extent((room_size_t)(num_room_sizes - 1));
	const extent_t image_dimensions = extent_scale(largest_room_extent, tile_texel_length);

	const queue_family_set_t queue_family_set = {
		.num_queue_families = 2,
		.queue_families = (uint32_t[2]){
			*physical_device.queue_family_indices.transfer_family_ptr,
			*physical_device.queue_family_indices.compute_family_ptr,
		}
	};

	const image_create_info_t compute_room_Textureransfer_image_create_info = {
		.num_image_array_layers = num_room_layers,
		.image_dimensions = image_dimensions,
		.format = VK_FORMAT_R8G8B8A8_UINT,
		.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		.queue_family_set = queue_family_set,
		.memory_type_index = memory_type_set.graphics_resources,
		.physical_device = physical_device.handle,
		.device = device
	};

	computeRoomTextureTransferImage = create_image(compute_room_Textureransfer_image_create_info);

	// Initial transition room texture storage image to transfer destination layout.

	VkCommandBuffer transition_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transition_command_buffer);
	begin_command_buffer(transition_command_buffer, 0);

	VkImageMemoryBarrier image_memory_barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = NULL,
		.srcAccessMask = VK_ACCESS_NONE,
		.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_GENERAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = computeRoomTextureTransferImage.vkImage,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS
	};

	const VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	const VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

	vkCmdPipelineBarrier(transition_command_buffer, source_stage, destination_stage, 0,
			0, NULL,
			0, NULL,
			1, &image_memory_barrier);

	vkEndCommandBuffer(transition_command_buffer);

	VkSubmitInfo transition_submit_info = { 0 };
	transition_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	transition_submit_info.pNext = NULL;
	transition_submit_info.waitSemaphoreCount = 0;
	transition_submit_info.pWaitSemaphores = NULL;
	transition_submit_info.pWaitDstStageMask = NULL;
	transition_submit_info.commandBufferCount = 1;
	transition_submit_info.pCommandBuffers = &transition_command_buffer;
	transition_submit_info.signalSemaphoreCount = 0;
	transition_submit_info.pSignalSemaphores = NULL;

	vkQueueSubmit(graphics_queue, 1, &transition_submit_info, NULL);
	vkQueueWaitIdle(graphics_queue);

	computeRoomTextureTransferImage.layout = VK_IMAGE_LAYOUT_GENERAL;

	log_message(VERBOSE, "Done creating room texture transfer image.");
}

void init_compute_room_texture(const VkDevice vk_device) {
	log_message(VERBOSE, "Initializing compute room texture...");
	compute_room_texture_pipeline = create_compute_pipeline(vk_device, compute_room_texture_layout, ROOM_TEXTURE_SHADER_NAME);
	createRoomTextureTransferImage();
}

void terminate_compute_room_texture(void) {
	destroy_image(&computeRoomTextureTransferImage);
	destroy_compute_pipeline(&compute_room_texture_pipeline);
}

void compute_room_texture(const room_t room, const uint32_t cacheSlot, const Texture tilemapTexture, Texture *const pRoomTexture) {
	log_message(VERBOSE, "Computing room texture...");
	
	if (pRoomTexture == NULL) {
		return;
	}

	byte_t *mapped_memory = buffer_partition_map_memory(global_uniform_buffer_partition, 1);
	const uint64_t num_tiles = extent_area(room.extent);
	memcpy(mapped_memory, room.tiles, num_tiles * sizeof(tile_t));
	buffer_partition_unmap_memory(global_uniform_buffer_partition);

	const VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = compute_room_texture_pipeline.descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &compute_room_texture_pipeline.descriptor_set_layout
	};

	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	const VkResult allocate_descriptor_set_result = vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set);
	if (allocate_descriptor_set_result != 0) {
		logf_message(ERROR, "Error computing room texture: descriptor set allocation failed. (Error code: %i)", allocate_descriptor_set_result);
		return;
	}

	const VkDescriptorBufferInfo uniform_buffer_info = buffer_partition_descriptor_info(global_uniform_buffer_partition, 1);
	const VkDescriptorImageInfo tilemapTexture_info = make_descriptor_image_info(no_sampler, tilemapTexture.image.vkImageView, tilemapTexture.layout);
	const VkDescriptorImageInfo room_texture_storage_info = make_descriptor_image_info(imageSamplerDefault, computeRoomTextureTransferImage.vkImageView, computeRoomTextureTransferImage.layout);
	const VkDescriptorImageInfo storage_image_infos[2] = { tilemapTexture_info, room_texture_storage_info };

	VkWriteDescriptorSet write_descriptor_sets[2] = { { 0 } };
	
	write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[0].pNext = NULL;
	write_descriptor_sets[0].dstSet = descriptor_set;
	write_descriptor_sets[0].dstBinding = 0;
	write_descriptor_sets[0].dstArrayElement = 0;
	write_descriptor_sets[0].descriptorCount = 1;
	write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[0].pImageInfo = NULL;
	write_descriptor_sets[0].pBufferInfo = &uniform_buffer_info;
	write_descriptor_sets[0].pTexelBufferView = NULL;

	write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[1].pNext = NULL;
	write_descriptor_sets[1].dstSet = descriptor_set;
	write_descriptor_sets[1].dstBinding = 1;
	write_descriptor_sets[1].dstArrayElement = 0;
	write_descriptor_sets[1].descriptorCount = 2;
	write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write_descriptor_sets[1].pImageInfo = storage_image_infos;
	write_descriptor_sets[1].pBufferInfo = NULL;
	write_descriptor_sets[1].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device, 2, write_descriptor_sets, 0, NULL);

	VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, compute_command_pool, 1, &compute_command_buffer);
	begin_command_buffer(compute_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_room_texture_pipeline.handle);
	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_room_texture_pipeline.layout, 0, 1, &descriptor_set, 0, NULL);
	vkCmdDispatch(compute_command_buffer, room.extent.width, room.extent.length, 1);

	vkEndCommandBuffer(compute_command_buffer);
	submit_command_buffers_async(compute_queue, 1, &compute_command_buffer);
	vkFreeCommandBuffers(device, compute_command_pool, 1, &compute_command_buffer);

	const VkImageSubresourceRange room_texture_image_subresource_range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = cacheSlot * num_room_layers,
		.layerCount = num_room_layers
	};

	/* Transition the room texture image layout from shader read-only to transfer destination. */
	
	VkCommandBuffer transition_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transition_command_buffer);
	begin_command_buffer(transition_command_buffer, 0); {

		const VkImageMemoryBarrier2 image_memory_barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.pNext = NULL,
			.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = pRoomTexture->image.vkImage,
			.subresourceRange = room_texture_image_subresource_range
		};

		const VkDependencyInfo dependency_info = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = NULL,
			.dependencyFlags = 0,
			.memoryBarrierCount = 0,
			.pMemoryBarriers = NULL,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers = NULL,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &image_memory_barrier
		};

		vkCmdPipelineBarrier2(transition_command_buffer, &dependency_info);

	} vkEndCommandBuffer(transition_command_buffer);
	submit_command_buffers_async(graphics_queue, 1, &transition_command_buffer);
	pRoomTexture->layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	vkResetCommandBuffer(transition_command_buffer, 0);

	/* Transfer compute result image to room texture image. */
	
	log_message(VERBOSE, "Transfering computed room image to room texture...");
	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); {

		const VkImageSubresourceLayers source_image_subresource_layers = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = num_room_layers
		};

		const VkImageSubresourceLayers destination_image_subresource_layers = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = cacheSlot * num_room_layers,
			.layerCount = num_room_layers
		};

		const VkImageCopy2 image_copy_region = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
			.pNext = NULL,
			.srcSubresource = source_image_subresource_layers,
			.srcOffset = (VkOffset3D){ 0, 0, 0 },
			.dstSubresource = destination_image_subresource_layers,
			.dstOffset = (VkOffset3D){ 0, 0, 0 },
			.extent = (VkExtent3D){
				.width = room.extent.width * tile_texel_length,
				.height = room.extent.length * tile_texel_length,
				.depth = 1
			}
		};

		const VkCopyImageInfo2 copy_image_info = {
			.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
			.pNext = NULL,
			.srcImage = computeRoomTextureTransferImage.vkImage,
			.srcImageLayout = computeRoomTextureTransferImage.layout,
			.dstImage = pRoomTexture->image.vkImage,
			.dstImageLayout = pRoomTexture->layout,
			.regionCount = 1,
			.pRegions = &image_copy_region
		};

		vkCmdCopyImage2(transfer_command_buffer, &copy_image_info);

	} vkEndCommandBuffer(transfer_command_buffer);
	submit_command_buffers_async(graphics_queue, 1, &transfer_command_buffer);
	vkFreeCommandBuffers(device, render_command_pool, 1, &transfer_command_buffer);

	/* Transition the room texture image layout back from transfer destination to shader read-only. */
	
	begin_command_buffer(transition_command_buffer, 0); {

		const VkImageMemoryBarrier2 image_memory_barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.pNext = NULL,
			.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = pRoomTexture->image.vkImage,
			.subresourceRange = room_texture_image_subresource_range
		};

		const VkDependencyInfo dependency_info = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = NULL,
			.dependencyFlags = 0,
			.memoryBarrierCount = 0,
			.pMemoryBarriers = NULL,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers = NULL,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &image_memory_barrier
		};

		vkCmdPipelineBarrier2(transition_command_buffer, &dependency_info);

	} vkEndCommandBuffer(transition_command_buffer);
	submit_command_buffers_async(graphics_queue, 1, &transition_command_buffer);
	pRoomTexture->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkFreeCommandBuffers(device, render_command_pool, 1, &transition_command_buffer);

	log_message(VERBOSE, "Done computing room texture.");
}
