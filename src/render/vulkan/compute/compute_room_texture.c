#include "compute_room_texture.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "log/logging.h"
#include "render/render_config.h"

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



#define ROOM_MODEL_CACHE_SIZE 2
#define NUM_ROOM_SIZES 4
#define NUM_ROOM_TEXTURES (ROOM_MODEL_CACHE_SIZE * NUM_ROOM_SIZES)

static const uint32_t room_model_cache_size = ROOM_MODEL_CACHE_SIZE;
static const uint32_t num_room_sizes = NUM_ROOM_SIZES;
static const uint32_t num_room_textures = NUM_ROOM_TEXTURES;

// Contains M * N textures, 
// 	where M is the number of cached room models (max number of room models loaded simultaneously), 
// 	and N is the number of predefined room sizes (e.g. 16 * 10 tiles).
// Textures of the same room size are stored together as a single cache, and each cache is stored sequentially.
static texture_t room_textures[NUM_ROOM_TEXTURES];

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
static void create_room_texture(uint32_t room_model_cache, uint32_t cache_slot, VkCommandBuffer *transition_command_buffers) {

	logf_message(VERBOSE, "Creating room texture in cache %u, slot %u...", room_model_cache, cache_slot);

	const uint32_t index = room_model_cache * room_model_cache_size + cache_slot;

	const extent_t room_extent = room_size_to_extent((room_size_t)room_model_cache);

	room_textures[index].num_images = 1;
	room_textures[index].images = calloc(room_textures[index].num_images, sizeof(VkImage));
	room_textures[index].image_views = calloc(room_textures[index].num_images, sizeof(VkImageView));
	room_textures[index].animation_cycles = calloc(room_textures[index].num_images, sizeof(texture_animation_cycle_t));
	room_textures[index].format = VK_FORMAT_R8G8B8A8_SRGB;
	room_textures[index].layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	room_textures[index].memory = VK_NULL_HANDLE;
	room_textures[index].device = device;

	room_textures[index].images[0] = VK_NULL_HANDLE;
	room_textures[index].image_views[0] = VK_NULL_HANDLE;
	room_textures[index].animation_cycles[0].num_frames = 1;
	room_textures[index].animation_cycles[0].frames_per_second = 0;

	// Create image.
	VkImageCreateInfo image_create_info = { 0 };
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.pNext = NULL;
	image_create_info.flags = 0;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = room_textures[index].format;
	image_create_info.extent.width = room_extent.width * 16;
	image_create_info.extent.height = room_extent.length * 16;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult image_create_result = vkCreateImage(room_textures[index].device, &image_create_info, NULL, &room_textures[index].images[0]);
	if (image_create_result != VK_SUCCESS) {
		logf_message(ERROR, "Error creating room texture: image creation failed. (Error code: %i)", image_create_result);
		return;
	}

	// Query memory requirements for the room texture image.
	VkImageMemoryRequirementsInfo2 image_memory_requirements_info = { 0 };
	image_memory_requirements_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
	image_memory_requirements_info.pNext = NULL;
	image_memory_requirements_info.image = room_textures[index].images[0];

	VkMemoryRequirements2 image_memory_requirements = { 0 };
	image_memory_requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	image_memory_requirements.pNext = NULL;

	vkGetImageMemoryRequirements2(room_textures[index].device, &image_memory_requirements_info, &image_memory_requirements);

	// Allocate memory for the room texture.
	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = image_memory_requirements.memoryRequirements.size;
	allocate_info.memoryTypeIndex = memory_type_set.graphics_resources;

	VkResult memory_allocation_result = vkAllocateMemory(device, &allocate_info, NULL, &room_textures[index].memory);
	if (memory_allocation_result != VK_SUCCESS) {
		logf_message(ERROR, "Error creating room texture: failed to allocate memory. (Error code: %i)", memory_allocation_result);
		// TODO - clean up previous image objects and return here.
	}

	// Bind the room texture image to memory.
	VkBindImageMemoryInfo image_bind_info = { 0 };
	image_bind_info.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
	image_bind_info.pNext = NULL;
	image_bind_info.image = room_textures[index].images[0];
	image_bind_info.memory = room_textures[index].memory;
	image_bind_info.memoryOffset = 0;

	vkBindImageMemory2(device, 1, &image_bind_info);

	// Create the image view for the room texture.
	VkImageViewCreateInfo image_view_create_info = { 0 };
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.pNext = NULL;
	image_view_create_info.flags = 0;
	image_view_create_info.image = room_textures[index].images[0];
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	image_view_create_info.format = room_textures[index].format;
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.subresourceRange = image_subresource_range;

	VkResult image_view_create_result = vkCreateImageView(room_textures[index].device, &image_view_create_info, NULL, &room_textures[index].image_views[0]);
	if (image_view_create_result != VK_SUCCESS) {
		logf_message(ERROR, "Error loading texture: image view creation failed. (Error code: %i)", image_view_create_result);
		return;
	}

	// Transition the room texture image layout from undefined to transfer destination.

	VkCommandBufferBeginInfo transition_command_buffer_begin_info = { 0 };
	transition_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	transition_command_buffer_begin_info.pNext = NULL;
	transition_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	transition_command_buffer_begin_info.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(transition_command_buffers[index], &transition_command_buffer_begin_info);

	VkImageMemoryBarrier image_memory_barrier = { 0 };
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = NULL;
	image_memory_barrier.srcAccessMask = VK_ACCESS_NONE;
	image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = room_textures[index].images[0];
	image_memory_barrier.subresourceRange = image_subresource_range;

	VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	vkCmdPipelineBarrier(transition_command_buffers[index], source_stage, destination_stage, 0,
			0, NULL,
			0, NULL,
			1, &image_memory_barrier);

	vkEndCommandBuffer(transition_command_buffers[index]);

	room_textures[index].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

static void create_room_textures(void) {
	
	log_message(VERBOSE, "Creating room textures...");

	// Allocate the command buffers to which will the image layout transitions be recorded.
	VkCommandBufferAllocateInfo transition_command_buffer_allocate_info = { 0 };
	transition_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	transition_command_buffer_allocate_info.pNext = NULL;
	transition_command_buffer_allocate_info.commandPool = render_command_pool;
	transition_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transition_command_buffer_allocate_info.commandBufferCount = num_room_textures;

	VkCommandBuffer transition_command_buffers[NUM_ROOM_TEXTURES];
	vkAllocateCommandBuffers(device, &transition_command_buffer_allocate_info, transition_command_buffers);

	for (uint32_t i = 0; i < num_room_sizes; ++i) {
		for (uint32_t j = 0; j < room_model_cache_size; ++j) {
			create_room_texture(i, j, transition_command_buffers);
		}
	}

	VkSubmitInfo submit_info = { 0 };
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.commandBufferCount = num_room_textures;
	submit_info.pCommandBuffers = transition_command_buffers;

	vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);
}

static void destroy_room_textures(void) {
	for (uint32_t i = 0; i < num_room_textures; ++i) {
		destroy_texture(room_textures[i]);
	}
}

void create_room_texture_transfer_image(void) {

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

void init_compute_room_texture(void) {

	log_message(VERBOSE, "Initializing compute room texture...");

	compute_pipeline_room_texture = create_compute_pipeline(device, compute_room_texture_layout, ROOM_TEXTURE_SHADER_NAME);
	create_room_textures();
	create_room_texture_transfer_image();
}

void terminate_compute_room_texture(void) {
	destroy_image(room_texture_transfer_image);
	destroy_room_textures();
	destroy_compute_pipeline(device, compute_pipeline_room_texture);
}

void compute_room_texture(texture_t tilemap_texture, uint32_t cache_slot, extent_t room_extent, uint32_t *tile_data) {

	log_message(VERBOSE, "Computing room texture...");

	uint32_t index = cache_slot;

	// TODO - temp solution, revamp this, possibly with enums.
	if (room_extent.width == 8 && room_extent.length == 5) {
		index += 0;
	}
	else if (room_extent.width == 16 && room_extent.length == 10) {
		index += 2;
	}
	else if (room_extent.width == 24 && room_extent.length == 15) {
		index += 4;
	}
	else if (room_extent.width == 32 && room_extent.length == 20) {
		index += 6;
	}
	else {
		log_message(ERROR, "Error computing room texture: room extent is invalid.");
	}

	const VkDeviceSize tile_data_size = 16 * room_extent.width * room_extent.length;

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
	const VkDescriptorImageInfo tilemap_texture_info = make_descriptor_image_info(no_sampler, tilemap_texture.image_views[0], tilemap_texture.layout);
	const VkDescriptorImageInfo room_texture_storage_info = make_descriptor_image_info(sampler_default, room_texture_transfer_image.view, room_texture_transfer_image.layout);

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

	VkDescriptorImageInfo storage_image_infos[2] = { tilemap_texture_info, room_texture_storage_info };

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

	vkCmdDispatch(compute_command_buffer, room_extent.width, room_extent.length, 1);

	vkEndCommandBuffer(compute_command_buffer);
	submit_command_buffers_async(compute_queue, 1, &compute_command_buffer);
	vkFreeCommandBuffers(device, compute_command_pool, 1, &compute_command_buffer);



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
	image_memory_barrier.image = room_textures[index].images[0];
	image_memory_barrier.subresourceRange = image_subresource_range;

	VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

	vkCmdPipelineBarrier(transition_command_buffer, source_stage, destination_stage, 0,
			0, NULL,
			0, NULL,
			1, &image_memory_barrier);

	vkEndCommandBuffer(transition_command_buffer);
	submit_command_buffers_async(graphics_queue, 1, &transition_command_buffer);
	room_textures[index].layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	vkResetCommandBuffer(transition_command_buffer, 0);

	// Transfer compute result image to room texture image.
	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, render_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageCopy copy_region = { 0 };
	copy_region.srcSubresource = image_subresource_layers_default;
	copy_region.srcOffset = (VkOffset3D){ 0, 0, 0 };
	copy_region.dstSubresource = image_subresource_layers_default;
	copy_region.dstOffset = (VkOffset3D){ 0, 0, 0 };
	copy_region.extent = (VkExtent3D){ 
		.width = room_extent.width * 16, 
		.height = room_extent.length * 16, 
		.depth = 1 
	};

	vkCmdCopyImage(transfer_command_buffer, 
			room_texture_transfer_image.handle, room_texture_transfer_image.layout,
			room_textures[index].images[0], room_textures[index].layout, 
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
	image_memory_barrier.image = room_textures[index].images[0];
	image_memory_barrier.subresourceRange = image_subresource_range;

	source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	vkCmdPipelineBarrier(transition_command_buffer, source_stage, destination_stage, 0,
			0, NULL,
			0, NULL,
			1, &image_memory_barrier);

	vkEndCommandBuffer(transition_command_buffer);
	submit_command_buffers_async(graphics_queue, 1, &transition_command_buffer);
	room_textures[index].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkFreeCommandBuffers(device, render_command_pool, 1, &transition_command_buffer);

	room_textures[index].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

texture_t get_room_texture(uint32_t room_size, uint32_t cache_slot) {
	const uint32_t index = room_size * room_model_cache_size + cache_slot;
	return room_textures[index];
}
