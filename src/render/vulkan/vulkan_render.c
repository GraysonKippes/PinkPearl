#include "vulkan_render.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log/logging.h"
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



// TODO - collate storage buffers as much as possible, and move to vulkan_manager.

// Buffers for compute_matrices shader.
static buffer_t matrix_buffer;		// Storage - GPU only

// TODO - move this into general texture functionality.
static image_t room_texture_pbr;	// Graphics - GPU only

// TODO - temporary synchronization for buffer transfer operations.
static VkFence transfer_fences[NUM_FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE };

static void transfer_model_data(void);



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

	init_compute_matrices(device);
	init_compute_room_texture(device);
	init_compute_area_mesh(device);
	create_vulkan_render_buffers();
	create_pbr_texture();

	const VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	
	for (uint32_t i = 0; i < num_frames_in_flight; ++i) {
		const VkResult fence_create_result = vkCreateFence(device, &fence_create_info, NULL, &transfer_fences[i]);
		if (fence_create_result < 0) {
			logf_message(ERROR, "Error creating Vulkan render objects: fence creation failed (result code: %i).", fence_create_result);
		}
		else if (fence_create_result > 0) {
			logf_message(WARNING, "Warning creating Vulkan render objects: fence creation returned with warning (result code: %i).", fence_create_result);
		}
	}
}

void destroy_vulkan_render_objects(void) {

	vkDeviceWaitIdle(device);

	log_message(VERBOSE, "Destroying Vulkan render objects...");

	destroy_buffer(&matrix_buffer);
	destroy_image(room_texture_pbr);
	terminate_compute_room_texture();
	terminate_compute_matrices();
	destroy_textures();
}

// Copy the model data to the appropriate staging buffers, and signal the render engine to transfer that data to the buffers on the GPU.
void stage_model_data(uint32_t slot, model_t model) {

	byte_t *const mapped_memory = buffer_partition_map_memory(global_staging_buffer_partition, 0);

	// Vertex Data

	for (uint32_t i = 0; i < num_frames_in_flight; ++i) {
		frames[i].model_update_flags |= (1LL << (uint64_t)slot);
	}

	// Model size is standard rect model size (4 vertices * 5 sp-floats per vertex * 4 bytes per sp-float = 80 bytes)
	VkDeviceSize model_size = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	VkDeviceSize model_offset = slot * model_size;
	memcpy((mapped_memory + model_offset), model.vertices, model_size);

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

	memcpy((mapped_memory + indices_offset), rect_indices, indices_size);
	buffer_partition_unmap_memory(global_staging_buffer_partition);
	transfer_model_data();
}

static void transfer_model_data(void) {

	static VkCommandBuffer transfer_command_buffers[NUM_FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE };

	// TODO - use the timeline semaphore instead.
	vkWaitForFences(device, 1, transfer_fences, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, transfer_fences);

	vkFreeCommandBuffers(device, transfer_command_pool, num_frames_in_flight, transfer_command_buffers);
	allocate_command_buffers(device, transfer_command_pool, num_frames_in_flight, transfer_command_buffers);

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};

	for (uint32_t i = 0; i < num_frames_in_flight; ++i) {
		vkBeginCommandBuffer(transfer_command_buffers[i], &begin_info);
	}

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

	VkCommandBufferSubmitInfo command_buffer_submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSemaphoreSubmitInfo semaphore_wait_submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSemaphoreSubmitInfo semaphore_signal_submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSubmitInfo2 submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };

	for (uint32_t i = 0; i < num_frames_in_flight; ++i) {

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
	vkQueueSubmit2(transfer_queue, num_frames_in_flight, submit_infos, transfer_fences[0]);
}

// Send the drawing commands to the GPU to draw the frame.
void draw_frame(const float tick_delta_time, const vector3F_t camera_position, const projection_bounds_t projection_bounds) {

	uint32_t image_index = 0;

	vkWaitForFences(device, 1, &FRAME.fence_frame_ready, VK_TRUE, UINT64_MAX);

	const VkResult result = vkAcquireNextImageKHR(device, swapchain.handle, UINT64_MAX, FRAME.semaphore_image_available, VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		// TODO - error handling
	}

	// Compute matrices
	// Signal a semaphore when the entire batch in the compute queue is done being executed.
	compute_matrices(tick_delta_time, projection_bounds, camera_position, render_object_positions);

	// TODO - revise pre-render compute synchronization to use semaphores instead;
	// 	this would allow graphics command buffer record to happen on the CPU 
	// 	while the compute command buffers are still being executed on the GPU.
	vkQueueWaitIdle(compute_queue);
	vkResetFences(device, 1, &FRAME.fence_frame_ready);
	vkResetCommandBuffer(FRAME.command_buffer, 0);

	const VkDescriptorBufferInfo matrix_buffer_info = buffer_partition_descriptor_info(global_storage_buffer_partition, 0);

	VkWriteDescriptorSet descriptor_writes[3] = { { 0 } };

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
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(FRAME.command_buffer, 0, 1, &FRAME.model_buffer.handle, offsets);
	vkCmdBindIndexBuffer(FRAME.command_buffer, FRAME.index_buffer.handle, 0, VK_INDEX_TYPE_UINT16);

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {

		if (!is_render_object_slot_enabled(i)) {
			continue;
		}

		const uint32_t render_object_slot = i;
		const uint32_t current_animation_frame = render_object_texture_states[i].current_frame;
		vkCmdPushConstants(FRAME.command_buffer, graphics_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (sizeof render_object_slot), &render_object_slot);
		vkCmdPushConstants(FRAME.command_buffer, graphics_pipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t), sizeof(uint32_t), &current_animation_frame);

		uint32_t first_index = render_object_slot * num_indices_per_rect;
		vkCmdDrawIndexed(FRAME.command_buffer, num_indices_per_rect, 1, first_index, 0, 0);
	}

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
