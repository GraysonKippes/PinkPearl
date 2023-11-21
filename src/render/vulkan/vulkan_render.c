#include "vulkan_render.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log/logging.h"
#include "render/render_config.h"
#include "render/render_object.h"
#include "util/time.h"

#include "command_buffer.h"
#include "texture.h"
#include "texture_manager.h"
#include "vertex_input.h"
#include "vulkan_manager.h"
#include "compute/compute_room_texture.h"



// Render object slots.
static uint64_t render_object_slot_enable_flags = 0;
static texture_t model_textures[NUM_RENDER_OBJECT_SLOTS];

static VkClearValue clear_color;

// TODO - collate storage buffers as much as possible, and move to vulkan_manager.

// Buffers for compute_matrices shader.
static buffer_t matrix_buffer;		// Storage - GPU only

// TODO - move this into general texture functionality.
static image_t room_texture_pbr;	// Graphics - GPU only



void create_vulkan_render_buffers(void) {

	log_message(VERBOSE, "Creating Vulkan render buffers...");

	static const VkDeviceSize num_matrices = NUM_RENDER_OBJECT_SLOTS + 2;
	static const VkDeviceSize matrix4F_size = 16 * sizeof(float);
	static const VkDeviceSize matrix_buffer_size = num_matrices * matrix4F_size;

	matrix_buffer = create_buffer(physical_device.handle, device, matrix_buffer_size, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			queue_family_set_null);
}

// TEST
void create_pbr_texture(void) {

	VkDeviceSize staging_buffer_size = 256 * 160 * 4;

	buffer_t image_staging_buffer_2 = create_buffer(physical_device.handle, device, staging_buffer_size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			queue_family_set_null);

	image_data_t image_data = load_image_data("../resources/assets/textures/pbr_test_2.png", 0);

	map_data_to_whole_buffer(device, image_staging_buffer_2, image_data.data);

	image_dimensions_t room_texture_dimensions = { (uint32_t)image_data.width, (uint32_t)image_data.height };

	queue_family_set_t queue_family_set_0 = {
		.num_queue_families = 2,
		.queue_families = (uint32_t[2]){
			*physical_device.queue_family_indices.graphics_family_ptr,
			*physical_device.queue_family_indices.transfer_family_ptr,
		}
	};

	room_texture_pbr = create_image(physical_device.handle, device, 
			room_texture_dimensions, 
			VK_FORMAT_R8G8B8A8_SRGB, true,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			queue_family_set_0);

	transition_image_layout(graphics_queue, render_command_pool, &room_texture_pbr, IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_TRANSFER_DST);

	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, transfer_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkBufferImageCopy2 copy_region = make_buffer_image_copy((VkOffset2D){ 0 }, (VkExtent2D){ 256, 160 });

	VkCopyBufferToImageInfo2 copy_info = { 0 };
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
	copy_info.pNext = NULL;
	copy_info.srcBuffer = image_staging_buffer_2.handle;
	copy_info.dstImage = room_texture_pbr.handle;
	copy_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	copy_info.regionCount = 1;
	copy_info.pRegions = &copy_region;

	vkCmdCopyBufferToImage2(transfer_command_buffer, &copy_info);

	vkEndCommandBuffer(transfer_command_buffer);
	submit_command_buffers_async(transfer_queue, 1, &transfer_command_buffer);
	vkFreeCommandBuffers(device, transfer_command_pool, 1, &transfer_command_buffer);

	destroy_buffer(&image_staging_buffer_2);
	free_image_data(image_data);

	transition_image_layout(graphics_queue, render_command_pool, &room_texture_pbr, IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_SHADER_READ_ONLY);
}

void create_vulkan_render_objects(void) {

	log_message(VERBOSE, "Creating Vulkan render objects...");

	init_compute_room_texture();
	create_vulkan_render_buffers();
	create_pbr_texture();
}

void destroy_vulkan_render_objects(void) {

	vkDeviceWaitIdle(device);

	destroy_buffer(&matrix_buffer);

	terminate_compute_room_texture();
}

bool is_render_object_slot_enabled(uint32_t slot) {
	return (render_object_slot_enable_flags & (1LL << (uint64_t)slot)) >= 1;
}

void enable_render_object_slot(uint32_t slot) {
	
	if (slot >= num_render_object_slots) {
		logf_message(ERROR, "Error enabling render object slot: render object slot (%u) exceeds total number of render object slots (%u).", slot, num_render_object_slots);
		return;
	}

	render_object_slot_enable_flags |= (1LL << (uint64_t)slot);
}

void disable_render_object_slot(uint32_t slot) {
	
	if (slot >= num_render_object_slots) {
		logf_message(ERROR, "Error disabling render object slot: render object slot (%u) exceeds total number of render object slots (%u).", slot, num_render_object_slots);
		return;
	}

	render_object_slot_enable_flags &= ~(1LL << (uint64_t)slot);
}

// Copy the model data to the appropriate staging buffers, and signal the render engine to transfer that data to the buffers on the GPU.
void stage_model_data(uint32_t slot, model_t model) {

	// Vertex Data

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkResetFences(device, 1, &frames[i].fence_buffers_up_to_date);
		frames[i].model_update_flags |= (1LL << (uint64_t)slot);
	}

	// Model size is standard rect model size (4 vertices * 5 sp-floats per vertex * 4 bytes per sp-float = 80 bytes)
	VkDeviceSize model_size = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	VkDeviceSize model_offset = slot * model_size;

	memcpy((global_staging_mapped_memory + model_offset), model.vertices, model_size);

	// Index Data

	VkDeviceSize base_offset = model_size * num_render_object_slots;
	VkDeviceSize indices_size = num_indices_per_rect * sizeof(index_t);
	VkDeviceSize indices_offset = base_offset + slot * indices_size;

	index_t rect_indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	for (uint32_t i = 0; i < num_indices_per_rect; ++i) {
		rect_indices[i] += slot * num_vertices_per_rect;
	}

	memcpy((global_staging_mapped_memory + indices_offset), rect_indices, indices_size);
}

void transfer_model_data(void) {

	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, transfer_command_pool, 1, &transfer_command_buffer);

	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkBufferCopy vertex_copy_regions[NUM_RENDER_OBJECT_SLOTS] = { { 0 } };
	VkBufferCopy index_copy_regions[NUM_RENDER_OBJECT_SLOTS] = { { 0 } };

	// The number of models needing to be updated.
	uint32_t num_pending_models = 0;

	const VkDeviceSize copy_size_vertices = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	const VkDeviceSize copy_size_indices = num_indices_per_rect * sizeof(index_t);

	const VkDeviceSize indices_base_offset = (VkDeviceSize)num_render_object_slots * copy_size_vertices;

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		
		if ((FRAME.model_update_flags & (1LL << (uint64_t)i)) == 0) {
			continue;
		}

		const uint32_t render_object_slot = i;

		vertex_copy_regions[num_pending_models].srcOffset = render_object_slot * copy_size_vertices;
		vertex_copy_regions[num_pending_models].dstOffset = render_object_slot * copy_size_vertices;
		vertex_copy_regions[num_pending_models].size = copy_size_vertices;

		index_copy_regions[num_pending_models].srcOffset = indices_base_offset + render_object_slot * copy_size_indices;
		index_copy_regions[num_pending_models].dstOffset = render_object_slot * copy_size_indices;
		index_copy_regions[num_pending_models].size = copy_size_indices;

		++num_pending_models;
	}

	FRAME.model_update_flags = 0;

	vkCmdCopyBuffer(transfer_command_buffer, global_staging_buffer, FRAME.model_buffer.handle, num_pending_models, vertex_copy_regions);
	vkCmdCopyBuffer(transfer_command_buffer, global_staging_buffer, FRAME.index_buffer.handle, num_pending_models, index_copy_regions);

	vkEndCommandBuffer(transfer_command_buffer);

	VkSubmitInfo submit_info = { 0 };
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &transfer_command_buffer;

	vkQueueSubmit(transfer_queue, 1, &submit_info, FRAME.fence_buffers_up_to_date);
	vkQueueWaitIdle(transfer_queue);

	vkFreeCommandBuffers(device, transfer_command_pool, 1, &transfer_command_buffer);
}

texture_t *get_model_texture_ptr(uint32_t slot) {
	
	if (slot >= num_render_object_slots) {
		logf_message(ERROR, "Error getting model texture: render object slot (%u) exceeds total number of render object slots (%u).", slot, num_render_object_slots);
		return NULL;
	}

	return model_textures + slot;
}

void set_model_texture(uint32_t slot, texture_t texture) {

	if (slot >= num_render_object_slots) {
		logf_message(ERROR, "Error setting model texture: render object slot (%u) exceeds total number of render object slots (%u).", slot, num_render_object_slots);
		return;
	}

	model_textures[slot] = texture;
	model_textures[slot].last_frame_time = get_time_ms();
}

void animate_texture(uint32_t slot) {

	uint64_t current_time_ms = get_time_ms();

	// Time difference between last frame change for this texture and current time, in seconds.
	const uint64_t delta_time_ms = current_time_ms - model_textures[slot].last_frame_time;
	const double delta_time = (double)delta_time_ms / 1000.0;

	// Frames per second of the current animation cycle of this texture, in frames per second.
	const double frames_per_second = (double)model_textures[slot].animation_cycles[model_textures[slot].current_animation_cycle].frames_per_second;

	// Time in seconds * frames per second = frames.
	const uint32_t frames = (uint32_t)(delta_time * frames_per_second);

	// Total number of frames in the current animation cycle of this texture.
	const uint32_t num_frames = model_textures[slot].animation_cycles[model_textures[slot].current_animation_cycle].num_frames;

	// Update the current frame and last frame time of this texture.
	model_textures[slot].current_animation_frame += frames;
	model_textures[slot].current_animation_frame %= num_frames;

	if (frames > 0) {
		model_textures[slot].last_frame_time = current_time_ms;
	}
}

// Dispatches a work load to the compute_matrices shader, computing a transformation matrix for each render object.
void compute_matrices(float delta_time, projection_bounds_t projection_bounds, render_position_t camera_position, render_position_t *positions) {

	static const VkDeviceSize uniform_data_size = 1024;

	byte_t *compute_matrices_data;
	vkMapMemory(device, global_uniform_memory, 0, uniform_data_size, 0, (void **)&compute_matrices_data);
	memcpy(compute_matrices_data, &delta_time, sizeof delta_time);
	memcpy(compute_matrices_data + 4, &projection_bounds, sizeof projection_bounds);
	memcpy(compute_matrices_data + 32, &camera_position, sizeof camera_position);
	memcpy(compute_matrices_data + 64, positions, num_render_object_slots * sizeof(render_position_t));
	vkUnmapMemory(device, global_uniform_memory);

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.pNext = NULL;
	descriptor_set_allocate_info.descriptorPool = compute_pipeline_matrices.descriptor_pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = &compute_pipeline_matrices.descriptor_set_layout;

	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set);

	VkDescriptorBufferInfo uniform_buffer_info = { 0 };
	uniform_buffer_info.buffer = global_uniform_buffer;
	uniform_buffer_info.offset = 0;
	uniform_buffer_info.range = uniform_data_size;

	VkDescriptorBufferInfo matrix_buffer_info = { 0 };
	matrix_buffer_info.buffer = matrix_buffer.handle;
	matrix_buffer_info.offset = 0;
	matrix_buffer_info.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet descriptor_writes[2] = { { 0 } };

	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &uniform_buffer_info;
	descriptor_writes[0].pImageInfo = NULL;
	descriptor_writes[0].pTexelBufferView = NULL;

	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pBufferInfo = &matrix_buffer_info;
	descriptor_writes[1].pImageInfo = NULL;
	descriptor_writes[1].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device, 2, descriptor_writes, 0, NULL);

	VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, compute_command_pool, 1, &compute_command_buffer);

	VkCommandBufferBeginInfo begin_info = { 0 };
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(compute_command_buffer, &begin_info);

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_matrices.handle);

	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_matrices.layout, 0, 1, &descriptor_set, 0, NULL);

	vkCmdDispatch(compute_command_buffer, 1, 1, 1);

	vkEndCommandBuffer(compute_command_buffer);
	submit_command_buffers_async(compute_queue, 1, &compute_command_buffer);
	vkFreeCommandBuffers(device, compute_command_pool, 1, &compute_command_buffer);

	vkResetDescriptorPool(device, compute_pipeline_matrices.descriptor_pool, 0);
}

void create_room_texture(room_t room, uint32_t render_object_slot) {

	// Create the properly aligned tile data array, aligned to 16 bytes.

	const uint64_t num_tiles = room.extent.width * room.extent.length;
	static const uint64_t tile_datum_size = 16;	// Bytes per tile datum--the buffer is aligned to 16 bytes.

	uint32_t *tile_data = calloc(num_tiles, tile_datum_size);
	if (tile_data == NULL) {
		log_message(ERROR, "Allocation of aligned tile data array failed.");
		return;
	}

	for (uint64_t i = 0; i < num_tiles; ++i) {
		// The indexing is in increments of sizeof(uint32_t), not of 1 byte;
		// 	therefore, multiply i by the alignment size (16 bytes) then divided i by sizeof(uint32_t).
		uint64_t index = i * (tile_datum_size / sizeof(uint32_t));
		tile_data[index] = room.tiles[i].tilemap_slot;
	}

	compute_room_texture(get_loaded_texture(1), render_object_slot, room.extent, tile_data);
	model_textures[render_object_slot] = get_room_texture((uint32_t)room.size, render_object_slot);

	free(tile_data);
}

void set_clear_color(color3F_t color) {
	clear_color.color.float32[0] = color.red;
	clear_color.color.float32[1] = color.green;
	clear_color.color.float32[2] = color.blue;
	clear_color.color.float32[3] = color.alpha;
}

// Send the drawing commands to the GPU to draw the frame.
void draw_frame(double delta_time, projection_bounds_t projection_bounds) {

	uint32_t image_index = 0;

	vkWaitForFences(device, 1, &FRAME.fence_frame_ready, VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(device, swapchain.handle, UINT64_MAX, FRAME.semaphore_image_available, VK_NULL_HANDLE, &image_index);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		// TODO - error handling
	}

	// If the buffers of this frame are not up-to-date, update them.
	if (vkGetFenceStatus(device, FRAME.fence_buffers_up_to_date) == VK_NOT_READY) {
		transfer_model_data();
	}


	// Compute matrices
	// Signal a semaphore when the entire batch in the compute queue is done being executed.

	render_position_t camera_position = { 0 };

	compute_matrices((float)delta_time, projection_bounds, camera_position, render_object_positions);



	// TODO - revise pre-render compute synchronization to use semaphores instead;
	// 	this would allow graphics command buffer record to happen on the CPU 
	// 	while the compute command buffers are still being executed on the GPU.
	vkQueueWaitIdle(compute_queue);
	
	vkWaitForFences(device, 1, &FRAME.fence_buffers_up_to_date, VK_TRUE, UINT64_MAX);

	vkResetFences(device, 1, &FRAME.fence_frame_ready);

	vkResetCommandBuffer(FRAME.command_buffer, 0);

	VkWriteDescriptorSet descriptor_writes[3] = { { 0 } };

	VkDescriptorBufferInfo matrix_buffer_info = { 0 };
	matrix_buffer_info.buffer = matrix_buffer.handle;
	matrix_buffer_info.offset = 0;
	matrix_buffer_info.range = VK_WHOLE_SIZE;

	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = FRAME.descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &matrix_buffer_info;
	descriptor_writes[0].pImageInfo = NULL;
	descriptor_writes[0].pTexelBufferView = NULL;

	VkDescriptorImageInfo texture_infos[NUM_RENDER_OBJECT_SLOTS];

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {

		if (is_render_object_slot_enabled(i)) {
			animate_texture(i);
			texture_infos[i].sampler = sampler_default;
			texture_infos[i].imageView = model_textures[i].image_views[model_textures[i].current_animation_cycle];
			texture_infos[i].imageLayout = model_textures[i].layout;
		}
		else {
			texture_t missing_texture = get_loaded_texture(0);
			texture_infos[i].sampler = sampler_default;
			texture_infos[i].imageView = missing_texture.image_views[0];
			texture_infos[i].imageLayout = missing_texture.layout;
		}
	}
	
	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = FRAME.descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[1].descriptorCount = num_render_object_slots;
	descriptor_writes[1].pBufferInfo = NULL;
	descriptor_writes[1].pImageInfo = texture_infos;
	descriptor_writes[1].pTexelBufferView = NULL;

	VkDescriptorImageInfo room_texture_pbr_info = { 0 };
	room_texture_pbr_info.sampler = sampler_default;
	room_texture_pbr_info.imageView = room_texture_pbr.view;
	room_texture_pbr_info.imageLayout = room_texture_pbr.layout;

	descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[2].dstSet = FRAME.descriptor_set;
	descriptor_writes[2].dstBinding = 2;
	descriptor_writes[2].dstArrayElement = 0;
	descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[2].descriptorCount = 1;
	descriptor_writes[2].pBufferInfo = NULL;
	descriptor_writes[2].pImageInfo = &room_texture_pbr_info;
	descriptor_writes[2].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device, 3, descriptor_writes, 0, NULL);



	VkCommandBufferBeginInfo begin_info = { 0 };
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(FRAME.command_buffer, &begin_info);

	VkRenderPassBeginInfo render_pass_info = { 0 };
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.pNext = NULL;
	render_pass_info.renderPass = graphics_pipeline.render_pass;
	render_pass_info.framebuffer = swapchain.framebuffers[image_index];
	render_pass_info.renderArea.offset.x = 0;
	render_pass_info.renderArea.offset.y = 0;
	render_pass_info.renderArea.extent = swapchain.extent;
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;

	vkCmdBeginRenderPass(FRAME.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(FRAME.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.handle);

	vkCmdBindDescriptorSets(FRAME.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.layout, 0, 1, &FRAME.descriptor_set, 0, NULL);

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(FRAME.command_buffer, 0, 1, &FRAME.model_buffer.handle, offsets);

	vkCmdBindIndexBuffer(FRAME.command_buffer, FRAME.index_buffer.handle, 0, VK_INDEX_TYPE_UINT16);

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {

		if (!is_render_object_slot_enabled(i)) {
			continue;
		}

		const uint32_t render_object_slot = i;
		vkCmdPushConstants(FRAME.command_buffer, graphics_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (sizeof render_object_slot), &render_object_slot);

		const uint32_t current_animation_frame = model_textures[i].current_animation_frame;
		vkCmdPushConstants(FRAME.command_buffer, graphics_pipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t), sizeof(uint32_t), &current_animation_frame);

		uint32_t first_index = render_object_slot * num_indices_per_rect;

		vkCmdDrawIndexed(FRAME.command_buffer, num_indices_per_rect, 1, first_index, 0, 0);
	}

	vkCmdEndRenderPass(FRAME.command_buffer);

	vkEndCommandBuffer(FRAME.command_buffer);



	VkPipelineStageFlags wait_stages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submit_info = { 0 };
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &FRAME.command_buffer;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &FRAME.semaphore_image_available;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &FRAME.semaphore_render_finished;

	if (vkQueueSubmit(graphics_queue, 1, &submit_info, FRAME.fence_frame_ready) != VK_SUCCESS) {
		// TODO - error handling
	}



	VkPresentInfoKHR present_info = { 0 };
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &FRAME.semaphore_render_finished;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain.handle;
	present_info.pImageIndices = &image_index;
	present_info.pResults = NULL;

	vkQueuePresentKHR(present_queue, &present_info);

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}
