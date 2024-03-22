#include "vulkan_render.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log/logging.h"
#include "game/area/room.h"
#include "render/render_config.h"
#include "util/time.h"

#include "command_buffer.h"
#include "texture.h"
#include "texture_manager.h"
#include "vertex_input.h"
#include "vulkan_manager.h"
#include "compute/compute_matrices.h"
#include "compute/compute_room_texture.h"
#include "compute/compute_area_mesh.h"

static void transfer_model_data(void);

void create_vulkan_render_objects(void) {

	log_message(VERBOSE, "Creating Vulkan render objects...");

	init_compute_matrices(device);
	init_compute_room_texture(device);
	init_compute_area_mesh(device);
}

void destroy_vulkan_render_objects(void) {

	vkDeviceWaitIdle(device);

	log_message(VERBOSE, "Destroying Vulkan render objects...");

	terminate_compute_room_texture();
	terminate_compute_matrices();
	destroy_textures();
}

// Copy the model data to the appropriate staging buffers, and signal the render engine to transfer that data to the buffers on the GPU.
void stage_model_data(uint32_t slot, model_t model) {

	// Vertex Data

	for (uint32_t i = 0; i < num_frames_in_flight; ++i) {
		frames[i].model_update_flags |= (1LL << (uint64_t)slot);
	}

	// Model size is standard rect model size (4 vertices * 5 sp-floats per vertex * 4 bytes per sp-float = 80 bytes)
	VkDeviceSize model_size = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	VkDeviceSize model_offset = slot * model_size;

	byte_t *const mapped_memory_vertices = buffer_partition_map_memory(global_staging_buffer_partition, 0);
	memcpy((mapped_memory_vertices + model_offset), model.vertices, model_size);
	buffer_partition_unmap_memory(global_staging_buffer_partition);

	// Index Data

	VkDeviceSize indices_size = num_indices_per_rect * sizeof(index_t);
	VkDeviceSize indices_offset = slot * indices_size;

	index_t rect_indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	for (uint32_t i = 0; i < num_indices_per_rect; ++i) {
		rect_indices[i] += slot * num_vertices_per_rect;
	}

	byte_t *const mapped_memory_indices = buffer_partition_map_memory(global_staging_buffer_partition, 1);
	memcpy((mapped_memory_indices + indices_offset), rect_indices, indices_size);
	buffer_partition_unmap_memory(global_staging_buffer_partition);
	transfer_model_data();
}

static void transfer_model_data(void) {

	// TODO - use deletion queue to delete command buffers after execution, instead of reusing same command buffers.

	static VkCommandBuffer transfer_command_buffers[NUM_FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE };

	VkSemaphore wait_semaphores[NUM_FRAMES_IN_FLIGHT];
	uint64_t wait_semaphore_values[NUM_FRAMES_IN_FLIGHT];

	for (uint32_t i = 0; i < num_frames_in_flight; ++i) {
		wait_semaphores[i] = frames[i].semaphore_buffers_ready.semaphore;
		wait_semaphore_values[i] = frames[i].semaphore_buffers_ready.wait_counter;
	}

	const VkSemaphoreWaitInfo semaphore_wait_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.pNext = NULL,
		.flags = 0,
		.semaphoreCount = num_frames_in_flight,
		.pSemaphores = wait_semaphores,
		.pValues = wait_semaphore_values
	};

	vkWaitSemaphores(device, &semaphore_wait_info, UINT64_MAX);
	vkFreeCommandBuffers(device, transfer_command_pool, num_frames_in_flight, transfer_command_buffers);
	allocate_command_buffers(device, transfer_command_pool, num_frames_in_flight, transfer_command_buffers);

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};

	VkBufferCopy vertex_copy_regions[NUM_RENDER_OBJECT_SLOTS] = { { 0 } };
	VkBufferCopy index_copy_regions[NUM_RENDER_OBJECT_SLOTS] = { { 0 } };

	// The number of models needing to be updated.
	uint32_t num_pending_models = 0;

	const VkDeviceSize copy_size_vertices = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	const VkDeviceSize copy_size_indices = num_indices_per_rect * sizeof(index_t);
	const VkDeviceSize indices_base_offset = global_staging_buffer_partition.ranges[1].offset;

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

	VkCommandBufferSubmitInfo command_buffer_submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSemaphoreSubmitInfo semaphore_wait_submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSemaphoreSubmitInfo semaphore_signal_submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSubmitInfo2 submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };

	for (uint32_t i = 0; i < num_frames_in_flight; ++i) {

		vkBeginCommandBuffer(transfer_command_buffers[i], &begin_info);
		vkCmdCopyBuffer(transfer_command_buffers[i], global_staging_buffer_partition.buffer, frames[i].model_buffer.handle, num_pending_models, vertex_copy_regions);
		vkCmdCopyBuffer(transfer_command_buffers[i], global_staging_buffer_partition.buffer, frames[i].index_buffer.handle, num_pending_models, index_copy_regions);
		vkEndCommandBuffer(transfer_command_buffers[i]);

		command_buffer_submit_infos[i] = make_command_buffer_submit_info(transfer_command_buffers[i]);
		semaphore_wait_submit_infos[i] = make_timeline_semaphore_wait_submit_info(frames[i].semaphore_render_finished, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		semaphore_signal_submit_infos[i] = make_timeline_semaphore_signal_submit_info(frames[i].semaphore_buffers_ready, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		frames[i].semaphore_buffers_ready.wait_counter += 1;

		submit_infos[i] = (VkSubmitInfo2){
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext = NULL,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &semaphore_wait_submit_infos[i],
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &command_buffer_submit_infos[i],
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &semaphore_signal_submit_infos[i]
		};
	}
	vkQueueSubmit2(transfer_queue, num_frames_in_flight, submit_infos, VK_NULL_HANDLE);
}

void upload_draw_data(const area_render_state_t area_render_state) {
	
	// Needs to be called in the following situations:
	// 	Texture is animated.
	// 	New render object is created/used.
	//	Any render object is destroyed/released.
	//	Area render enters/exits scroll state.
	
	typedef struct draw_info_t {
		// Indirect draw info
		uint32_t index_count;
		uint32_t instance_count;
		uint32_t first_index;
		int32_t vertex_offset;
		uint32_t first_instance;
		// Additional draw info
		uint32_t render_object_slot;
		uint32_t image_index;
	} draw_info_t;
	
	uint32_t draw_count = 0;
	const uint32_t max_draw_count = NUM_RENDER_OBJECT_SLOTS + NUM_ROOM_TEXTURE_CACHE_SLOTS * NUM_ROOM_LAYERS;
	draw_info_t draw_infos[NUM_RENDER_OBJECT_SLOTS + NUM_ROOM_TEXTURE_CACHE_SLOTS * NUM_ROOM_LAYERS];
	
	for (uint32_t i = 0; i < max_draw_count; ++i) {
		draw_infos[i] = (draw_info_t){
			.index_count = 6,
			.instance_count = 1,
			.first_index = 0,
			.vertex_offset = 0,
			.first_instance = 0,
			.render_object_slot = 0,
			.image_index = 0
		};
	}

	const uint32_t current_room_id = area_render_state.cache_slots_to_room_ids[area_render_state.current_cache_slot];
	const uint32_t next_room_id = area_render_state.cache_slots_to_room_ids[area_render_state.next_cache_slot];
	
	// Current room background
	draw_infos[draw_count++] = (draw_info_t){
		.index_count = 6,
		.instance_count = 1,
		.first_index = 0,
		.vertex_offset = (int32_t)((num_render_object_slots + current_room_id) * num_vertices_per_rect),
		.first_instance = 0,
		.render_object_slot = num_render_object_slots + (uint32_t)area_render_state.room_size,
		.image_index = area_render_state.current_cache_slot * num_room_layers
	};
	
	// Next room background
	if (area_render_state_is_scrolling()) {
		draw_infos[draw_count++] = (draw_info_t){
			.index_count = 6,
			.instance_count = 1,
			.first_index = 0,
			.vertex_offset = (int32_t)((num_render_object_slots + next_room_id) * num_vertices_per_rect),
			.first_instance = 0,
			.render_object_slot = num_render_object_slots + (uint32_t)area_render_state.room_size,
			.image_index = area_render_state.next_cache_slot * num_room_layers
		};
	}
	
	// Render objects
	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		if (!is_render_object_slot_enabled(i)) {
			continue;
		}
		
		draw_infos[draw_count++] = (draw_info_t){
			.index_count = 6,
			.instance_count = 1,
			.first_index = 0,
			.vertex_offset = (int32_t)(i * num_vertices_per_rect),
			.first_instance = 0,
			.render_object_slot = i,
			.image_index = render_object_texture_states[i].current_frame
		};
	}
	
	// Current room foreground
	draw_infos[draw_count++] = (draw_info_t){
		.index_count = 6,
		.instance_count = 1,
		.first_index = 0,
		.vertex_offset = (int32_t)((num_render_object_slots + current_room_id) * num_vertices_per_rect),
		.first_instance = 0,
		.render_object_slot = num_render_object_slots + (uint32_t)area_render_state.room_size,
		.image_index = area_render_state.current_cache_slot * num_room_layers + 1
	};
	
	// Next room foreground
	if (area_render_state_is_scrolling()) {
		draw_infos[draw_count++] = (draw_info_t){
			.index_count = 6,
			.instance_count = 1,
			.first_index = 0,
			.vertex_offset = (int32_t)((num_render_object_slots + next_room_id) * num_vertices_per_rect),
			.first_instance = 0,
			.render_object_slot = num_render_object_slots + (uint32_t)area_render_state.room_size,
			.image_index = area_render_state.next_cache_slot * num_room_layers + 1
		};
	}
	
	byte_t *draw_count_mapped_memory = buffer_partition_map_memory(global_draw_data_buffer_partition, 0);
	memcpy(draw_count_mapped_memory, &draw_count, sizeof(draw_count));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
	
	byte_t *draw_data_mapped_memory = buffer_partition_map_memory(global_draw_data_buffer_partition, 1);
	memcpy(draw_data_mapped_memory, draw_infos, max_draw_count * sizeof(draw_info_t));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
}

void draw_frame(const float tick_delta_time, const vector3F_t camera_position, const projection_bounds_t projection_bounds, const area_render_state_t area_render_state) {

	vkWaitForFences(device, 1, &FRAME.fence_frame_ready, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &FRAME.fence_frame_ready);
	vkResetCommandBuffer(FRAME.command_buffer, 0);

	uint32_t image_index = 0;
	const VkResult result = vkAcquireNextImageKHR(device, swapchain.handle, UINT64_MAX, FRAME.semaphore_image_available, VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		// TODO - error handling
	}

	// Signal a semaphore when the entire batch in the compute queue is done being executed.
	compute_matrices(tick_delta_time, projection_bounds, camera_position, render_object_positions);

	VkWriteDescriptorSet descriptor_writes[3] = { { 0 } };

	const VkDescriptorBufferInfo draw_data_buffer_info = buffer_partition_descriptor_info(global_draw_data_buffer_partition, 1);
	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = FRAME.descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &draw_data_buffer_info;
	descriptor_writes[0].pImageInfo = NULL;
	descriptor_writes[0].pTexelBufferView = NULL;

	const VkDescriptorBufferInfo matrix_buffer_info = buffer_partition_descriptor_info(global_storage_buffer_partition, 0);
	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = FRAME.descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pBufferInfo = &matrix_buffer_info;
	descriptor_writes[1].pImageInfo = NULL;
	descriptor_writes[1].pTexelBufferView = NULL;

	VkDescriptorImageInfo texture_infos[NUM_RENDER_OBJECT_SLOTS + NUM_ROOM_SIZES];

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		if (is_render_object_slot_enabled(i)) {
			const texture_state_t texture_state = render_object_texture_states[i];
			const texture_t texture = get_loaded_texture(texture_state.handle);
			texture_infos[i].sampler = sampler_default;
			texture_infos[i].imageView = texture.images[texture_state.current_animation_cycle].vk_image_view;
			texture_infos[i].imageLayout = texture.layout;
		}
		else {
			const texture_t missing_texture = get_loaded_texture(missing_texture_handle);
			texture_infos[i].sampler = sampler_default;
			texture_infos[i].imageView = missing_texture.images[0].vk_image_view;
			texture_infos[i].imageLayout = missing_texture.layout;
		}
	}
	
	for (uint32_t i = 0; i < num_room_sizes; ++i) {
		const texture_t room_texture = get_loaded_texture(room_texture_handle);
		const uint32_t index = num_render_object_slots + i;
		texture_infos[index].sampler = sampler_default;
		texture_infos[index].imageView = room_texture.images[i].vk_image_view;
		texture_infos[index].imageLayout = room_texture.layout;
	}

	descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[2].dstSet = FRAME.descriptor_set;
	descriptor_writes[2].dstBinding = 2;
	descriptor_writes[2].dstArrayElement = 0;
	descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[2].descriptorCount = num_render_object_slots + num_room_sizes;
	descriptor_writes[2].pBufferInfo = NULL;
	descriptor_writes[2].pImageInfo = texture_infos;
	descriptor_writes[2].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device, 3, descriptor_writes, 0, NULL);



	VkCommandBufferBeginInfo begin_info = { 0 };
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(FRAME.command_buffer, &begin_info);

	static const VkClearValue clear_value = { { { 0 } } };

	VkRenderPassBeginInfo render_pass_info = { 0 };
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.pNext = NULL;
	render_pass_info.renderPass = graphics_pipeline.render_pass;
	render_pass_info.framebuffer = swapchain.framebuffers[image_index];
	render_pass_info.renderArea.offset.x = 0;
	render_pass_info.renderArea.offset.y = 0;
	render_pass_info.renderArea.extent = swapchain.extent;
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_value;

	vkCmdBeginRenderPass(FRAME.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(FRAME.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.handle);
	vkCmdBindDescriptorSets(FRAME.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.layout, 0, 1, &FRAME.descriptor_set, 0, NULL);
	const VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(FRAME.command_buffer, 0, 1, &FRAME.model_buffer.handle, offsets);
	vkCmdBindIndexBuffer(FRAME.command_buffer, FRAME.index_buffer.handle, 0, VK_INDEX_TYPE_UINT16);
	
	const uint32_t draw_data_offset = global_draw_data_buffer_partition.ranges[1].offset;
	vkCmdDrawIndexedIndirectCount(FRAME.command_buffer, global_draw_data_buffer_partition.buffer, draw_data_offset, global_draw_data_buffer_partition.buffer, 0, 68, 28);

	vkCmdEndRenderPass(FRAME.command_buffer);
	vkEndCommandBuffer(FRAME.command_buffer);



	const VkCommandBufferSubmitInfo command_buffer_submit_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = NULL,
		.commandBuffer = FRAME.command_buffer,
		.deviceMask = 0
	};

	VkSemaphoreSubmitInfo wait_semaphore_submit_infos[3] = { 0 };
	wait_semaphore_submit_infos[1] = make_timeline_semaphore_wait_submit_info(FRAME.semaphore_buffers_ready, VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT);

	wait_semaphore_submit_infos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_semaphore_submit_infos[0].pNext = NULL;
	wait_semaphore_submit_infos[0].semaphore = FRAME.semaphore_image_available;
	wait_semaphore_submit_infos[0].value = 0;
	wait_semaphore_submit_infos[0].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	wait_semaphore_submit_infos[0].deviceIndex = 0;

	wait_semaphore_submit_infos[2].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_semaphore_submit_infos[2].pNext = NULL;
	wait_semaphore_submit_infos[2].semaphore = compute_matrices_semaphore;
	wait_semaphore_submit_infos[2].value = 0;
	wait_semaphore_submit_infos[2].stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
	wait_semaphore_submit_infos[2].deviceIndex = 0;

	VkSemaphoreSubmitInfo signal_semaphore_submit_infos[1] = { 0 };
	signal_semaphore_submit_infos[0] = make_timeline_semaphore_signal_submit_info(FRAME.semaphore_render_finished, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
	FRAME.semaphore_render_finished.wait_counter += 1;

	const VkSubmitInfo2 submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = NULL,
		.flags = 0,
		.waitSemaphoreInfoCount = 3,
		.pWaitSemaphoreInfos = wait_semaphore_submit_infos,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &command_buffer_submit_info,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = signal_semaphore_submit_infos
	};

	vkQueueSubmit2(graphics_queue, 1, &submit_info, FRAME.fence_frame_ready);



	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &FRAME.semaphore_present_ready.semaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapchain.handle,
		.pImageIndices = &image_index,
		.pResults = NULL
	};

	timeline_to_binary_semaphore_signal(graphics_queue, FRAME.semaphore_render_finished, FRAME.semaphore_present_ready);
	vkQueuePresentKHR(present_queue, &present_info);

	current_frame = (current_frame + 1) % num_frames_in_flight;
}
