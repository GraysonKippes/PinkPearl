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

static compute_pipeline_t compute_pipeline_room_texture;

static image_t room_texture_transfer_image;

// Subresource range used in all image views and layout transitions.
static const VkImageSubresourceRange image_subresource_range = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseMipLevel = 0,
	.levelCount = 1,
	.baseArrayLayer = 0,
	.layerCount = VK_REMAINING_ARRAY_LAYERS
};



// Creates a room texture and returns the command buffer recorded with the image layout transition.
texture_t init_room_texture(const room_size_t room_size, const uint32_t cache_slot) {

	logf_message(VERBOSE, "Creating room texture in cache %u, slot %u...", (uint32_t)room_size, cache_slot);

	const extent_t room_extent = room_size_to_extent(room_size);

	texture_create_info_t room_texture_create_info = { 0 };
	room_texture_create_info.path = "missing.png";
	room_texture_create_info.atlas_extent.width = 16 * room_extent.width;
	room_texture_create_info.atlas_extent.length = 16 * room_extent.length;
	room_texture_create_info.num_cells.width = 1;
	room_texture_create_info.num_cells.length = 1;
	room_texture_create_info.cell_extent.width = room_texture_create_info.atlas_extent.width;
	room_texture_create_info.cell_extent.length = room_texture_create_info.atlas_extent.length;
	room_texture_create_info.num_animations = 1;
	room_texture_create_info.animations = (animation_create_info_t[1]){
		{
			.start_cell = 0,
			.num_frames = room_texture_cache_size,
			.frames_per_second = 0
		}
	};

	texture_t room_texture = create_texture(room_texture_create_info);
	if (is_texture_null(room_texture)) {
		log_message(ERROR, "Error creating room texture: texture creation failed.");
		return room_texture;
	}

	// Transition the room texture image layout from undefined to shader read-only optimal.

	VkCommandBuffer transition_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(room_texture.device, render_command_pool, 1, &transition_command_buffer);

	VkCommandBufferBeginInfo transition_command_buffer_begin_info = { 0 };
	transition_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	transition_command_buffer_begin_info.pNext = NULL;
	transition_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	transition_command_buffer_begin_info.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(transition_command_buffer, &transition_command_buffer_begin_info);

	VkImageMemoryBarrier image_memory_barrier = { 0 };
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = NULL;
	image_memory_barrier.srcAccessMask = VK_ACCESS_NONE;
	image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = room_texture.images[0].vk_image;
	image_memory_barrier.subresourceRange = image_subresource_range;

	VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

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

	room_texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	return room_texture;
}

static void create_room_texture_transfer_image(void) {

	log_message(VERBOSE, "Creating room texture transfer image...");

	// Dimensions for the largest possible room texture, smaller textures use a subsection of this image.
	static const image_dimensions_t image_dimensions = {
		32 * 16,
		20 * 16
	};

	const queue_family_set_t queue_family_set = {
		.num_queue_families = 2,
		.queue_families = (uint32_t[2]){
			*physical_device.queue_family_indices.transfer_family_ptr,
			*physical_device.queue_family_indices.compute_family_ptr,
		}
	};

	static const VkFormat storage_image_format = VK_FORMAT_R8G8B8A8_UINT;

	room_texture_transfer_image = create_image(physical_device.handle, device, 
			image_dimensions, storage_image_format, false,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			queue_family_set);

	// Initial transition room texture storage image to transfer destination layout.

	VkCommandBuffer transition_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transition_command_buffer);
	begin_command_buffer(transition_command_buffer, 0);

	VkImageMemoryBarrier image_memory_barrier = { 0 };
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = NULL;
	image_memory_barrier.srcAccessMask = VK_ACCESS_NONE;
	image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = room_texture_transfer_image.handle;
	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

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

	room_texture_transfer_image.layout = VK_IMAGE_LAYOUT_GENERAL;
}

void init_compute_room_texture(const VkDevice vk_device) {

	log_message(VERBOSE, "Initializing compute room texture...");

	compute_pipeline_room_texture = create_compute_pipeline(vk_device, compute_room_texture_layout, ROOM_TEXTURE_SHADER_NAME);
	create_room_texture_transfer_image();
}

void terminate_compute_room_texture(void) {
	destroy_image(room_texture_transfer_image);
	destroy_compute_pipeline(&compute_pipeline_room_texture);
}

void compute_room_texture(const room_t room, const uint32_t cache_slot, const texture_t tilemap_texture, texture_t *const room_texture_ptr) {

	log_message(VERBOSE, "Computing room texture...");

	// Create the properly aligned tile data array, aligned to 16 bytes.
	static const uint64_t tile_datum_size = 16;	// Bytes per tile datum--the buffer is aligned to 16 bytes.
	const uint64_t num_tiles = room.extent.width * room.extent.length;
	const VkDeviceSize tile_data_size = num_tiles * tile_datum_size;
	uint32_t *tile_data = calloc(num_tiles, tile_datum_size);
	if (tile_data == NULL) {
		log_message(ERROR, "Error creating room texture: allocation of aligned tile data array failed.");
		return;
	}
	for (uint64_t i = 0; i < num_tiles; ++i) {
		// The indexing is in increments of sizeof(uint32_t), not of 1 byte;
		// 	therefore, multiply i by the alignment size (16 bytes) then divided i by sizeof(uint32_t).
		uint64_t index = i * (tile_datum_size / sizeof(uint32_t));
		tile_data[index] = room.tiles[i].tilemap_slot;
	}
	memcpy((global_uniform_mapped_memory + 128), tile_data, tile_data_size);

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.pNext = NULL;
	descriptor_set_allocate_info.descriptorPool = compute_pipeline_room_texture.descriptor_pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = &compute_pipeline_room_texture.descriptor_set_layout;

	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	const VkResult allocate_descriptor_set_result = vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set);
	if (allocate_descriptor_set_result != 0) {
		logf_message(ERROR, "Error computing room texture: descriptor set allocation failed. (Error code: %i)", allocate_descriptor_set_result);
		return;
	}

	const VkDescriptorBufferInfo uniform_buffer_info = make_descriptor_buffer_info(global_uniform_buffer, 128, tile_data_size);
	const VkDescriptorImageInfo tilemap_texture_info = make_descriptor_image_info(no_sampler, tilemap_texture.images[0].vk_image_view, tilemap_texture.layout);
	const VkDescriptorImageInfo room_texture_storage_info = make_descriptor_image_info(sampler_default, room_texture_transfer_image.view, room_texture_transfer_image.layout);
	const VkDescriptorImageInfo storage_image_infos[2] = { tilemap_texture_info, room_texture_storage_info };

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

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_room_texture.handle);
	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_room_texture.layout, 0, 1, &descriptor_set, 0, NULL);
	vkCmdDispatch(compute_command_buffer, room.extent.width, room.extent.length, 1);

	vkEndCommandBuffer(compute_command_buffer);
	submit_command_buffers_async(compute_queue, 1, &compute_command_buffer);
	vkFreeCommandBuffers(device, compute_command_pool, 1, &compute_command_buffer);



	const VkImageSubresourceRange image_subresource_range_2 = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = cache_slot,
		.layerCount = 1
	};

	// Transition the room texture image layout from shader read-only to transfer destination.
	VkCommandBuffer transition_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transition_command_buffer);
	begin_command_buffer(transition_command_buffer, 0);

	VkImageMemoryBarrier image_memory_barrier = { 0 };
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = NULL;
	image_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = room_texture_ptr->images[0].vk_image;
	image_memory_barrier.subresourceRange = image_subresource_range_2;

	VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

	vkCmdPipelineBarrier(transition_command_buffer, source_stage, destination_stage, 0,
			0, NULL,
			0, NULL,
			1, &image_memory_barrier);

	vkEndCommandBuffer(transition_command_buffer);
	submit_command_buffers_async(graphics_queue, 1, &transition_command_buffer);
	room_texture_ptr->layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	vkResetCommandBuffer(transition_command_buffer, 0);



	// Transfer compute result image to room texture image.
	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	const VkImageSubresourceLayers destination_image_subresource_layers = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.mipLevel = 0,
		.baseArrayLayer = cache_slot,
		.layerCount = 1
	};

	VkImageCopy copy_region = { 0 };
	copy_region.srcSubresource = image_subresource_layers_default;
	copy_region.srcOffset = (VkOffset3D){ 0, 0, 0 };
	copy_region.dstSubresource = destination_image_subresource_layers;
	copy_region.dstOffset = (VkOffset3D){ 0, 0, 0 };
	copy_region.extent = (VkExtent3D){ 
		.width = room.extent.width * 16, 
		.height = room.extent.length * 16, 
		.depth = 1 
	};

	vkCmdCopyImage(transfer_command_buffer, 
			room_texture_transfer_image.handle, room_texture_transfer_image.layout,
			room_texture_ptr->images[0].vk_image, room_texture_ptr->layout, 
			1, &copy_region);

	vkEndCommandBuffer(transfer_command_buffer);
	submit_command_buffers_async(graphics_queue, 1, &transfer_command_buffer);
	vkFreeCommandBuffers(device, render_command_pool, 1, &transfer_command_buffer);

	// Transition the room texture image layout back from transfer destination to shader read-only.
	begin_command_buffer(transition_command_buffer, 0);

	image_memory_barrier = (VkImageMemoryBarrier){ 0 };
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = NULL;
	image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = room_texture_ptr->images[0].vk_image;
	image_memory_barrier.subresourceRange = image_subresource_range_2;

	source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	vkCmdPipelineBarrier(transition_command_buffer, source_stage, destination_stage, 0,
			0, NULL,
			0, NULL,
			1, &image_memory_barrier);

	vkEndCommandBuffer(transition_command_buffer);
	submit_command_buffers_async(graphics_queue, 1, &transition_command_buffer);
	room_texture_ptr->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkFreeCommandBuffers(device, render_command_pool, 1, &transition_command_buffer);

	log_message(VERBOSE, "Done computing room texture.");
}
