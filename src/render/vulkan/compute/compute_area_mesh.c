#include "compute_area_mesh.h"

#include <stddef.h>
#include <string.h>

#include "log/logging.h"
#include "render/render_config.h"
#include "util/allocate.h"
#include "util/byte.h"
#include "math/offset.h"

#include "../command_buffer.h"
#include "../compute_pipeline.h"
#include "../descriptor.h"
#include "../vertex_input.h"
#include "../vulkan_manager.h"

static const descriptor_binding_t compute_area_mesh_bindings[3] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT }
};

static const descriptor_layout_t compute_area_mesh_layout = {
	.num_bindings = 3,
	.bindings = (descriptor_binding_t *)compute_area_mesh_bindings
};

static ComputePipeline compute_area_mesh_pipeline;

static const size_t max_area_extent_x = 32;
static const size_t max_area_extent_y = 32;
static const size_t max_num_rooms = max_area_extent_x * max_area_extent_y;
static const VkDeviceSize compute_area_mesh_uniform_size = sizeof(Extent) + max_num_rooms * sizeof(offset_t);

void init_compute_area_mesh(const VkDevice vk_device) {
	compute_area_mesh_pipeline = create_compute_pipeline(vk_device, compute_area_mesh_layout, AREA_MESH_SHADER_NAME);
}

void terminate_compute_area_mesh(void) {
	destroy_compute_pipeline(&compute_area_mesh_pipeline);
}

void compute_area_mesh(const area_t area) {
	log_message(VERBOSE, "Generating area mesh...");

	const uint32_t dispatch_x = area.num_rooms;
	const uint32_t dispatch_y = 1;
	const uint32_t dispatch_z = 1;

	offset_t *room_positions = nullptr;
	if (!allocate((void **)&room_positions, area.num_rooms, sizeof(offset_t))) {
		log_message(ERROR, "Error computing area mesh: failed to allocate room position pointer-array.");
		return;
	}

	for (uint32_t i = 0; i < area.num_rooms; ++i) {
		room_positions[i] = area.rooms[i].position;
	}

	byte_t *const mapped_memory = buffer_partition_map_memory(global_uniform_buffer_partition, 2);
	memcpy(mapped_memory, &area.room_extent, sizeof area.room_extent);
	memcpy(mapped_memory + sizeof area.room_extent, room_positions, (size_t)area.num_rooms * sizeof(offset_t));
	buffer_partition_unmap_memory(global_uniform_buffer_partition);
	
	const VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = compute_area_mesh_pipeline.descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &compute_area_mesh_pipeline.descriptor_set_layout
	};

	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	const VkResult allocate_descriptor_set_result = vkAllocateDescriptorSets(compute_area_mesh_pipeline.device, &descriptor_set_allocate_info, &descriptor_set);
	if (allocate_descriptor_set_result != 0) {
		logf_message(ERROR, "Error computing room texture: descriptor set allocation failed. (Error code: %i)", allocate_descriptor_set_result);
		return;
	}

	const VkDescriptorBufferInfo uniform_buffer_info = buffer_partition_descriptor_info(global_uniform_buffer_partition, 2);
	const VkDescriptorBufferInfo vertices_buffer_info = buffer_partition_descriptor_info(global_storage_buffer_partition, 1);
	const VkDescriptorBufferInfo indices_buffer_info = buffer_partition_descriptor_info(global_storage_buffer_partition, 2);

	VkWriteDescriptorSet write_descriptor_sets[3] = { { 0 } };
	
	write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[0].pNext = nullptr;
	write_descriptor_sets[0].dstSet = descriptor_set;
	write_descriptor_sets[0].dstBinding = 0;
	write_descriptor_sets[0].dstArrayElement = 0;
	write_descriptor_sets[0].descriptorCount = 1;
	write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[0].pImageInfo = nullptr;
	write_descriptor_sets[0].pBufferInfo = &uniform_buffer_info;
	write_descriptor_sets[0].pTexelBufferView = nullptr;

	write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[1].pNext = nullptr;
	write_descriptor_sets[1].dstSet = descriptor_set;
	write_descriptor_sets[1].dstBinding = 1;
	write_descriptor_sets[1].dstArrayElement = 0;
	write_descriptor_sets[1].descriptorCount = 1;
	write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write_descriptor_sets[1].pImageInfo = nullptr;
	write_descriptor_sets[1].pBufferInfo = &vertices_buffer_info;
	write_descriptor_sets[1].pTexelBufferView = nullptr;

	write_descriptor_sets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[2].pNext = nullptr;
	write_descriptor_sets[2].dstSet = descriptor_set;
	write_descriptor_sets[2].dstBinding = 2;
	write_descriptor_sets[2].dstArrayElement = 0;
	write_descriptor_sets[2].descriptorCount = 1;
	write_descriptor_sets[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write_descriptor_sets[2].pImageInfo = nullptr;
	write_descriptor_sets[2].pBufferInfo = &indices_buffer_info;
	write_descriptor_sets[2].pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(compute_area_mesh_pipeline.device, 3, write_descriptor_sets, 0, nullptr);

	VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, compute_command_pool, 1, &compute_command_buffer);
	cmdBufBegin(compute_command_buffer, true);

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_area_mesh_pipeline.handle);
	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_area_mesh_pipeline.layout, 0, 1, &descriptor_set, 0, nullptr);
	vkCmdDispatch(compute_command_buffer, dispatch_x, dispatch_y, dispatch_z);

	vkEndCommandBuffer(compute_command_buffer);
	submit_command_buffers_async(compute_queue, 1, &compute_command_buffer);
	vkFreeCommandBuffers(device, compute_command_pool, 1, &compute_command_buffer);

	/* TRANSFER */
	log_message(VERBOSE, "Transfering area mesh to graphics buffers.");

	VkCommandBuffer transfer_command_buffers[NUM_FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE };
	allocate_command_buffers(device, cmdPoolTransfer, num_frames_in_flight, transfer_command_buffers);

	const VkBufferCopy vertex_buffer_copy = { 
		.srcOffset = global_storage_buffer_partition.ranges[1].offset,
		// TODO - use frame buffer partition when it is implemented.
		.dstOffset = numRenderObjectSlots * num_vertices_per_rect * vertex_input_element_stride * sizeof(float),
		.size = global_storage_buffer_partition.ranges[1].size
		
	};

	const VkBufferCopy index_buffer_copy = { 
		.srcOffset = global_storage_buffer_partition.ranges[2].offset,
		.dstOffset = 768,
		.size = global_storage_buffer_partition.ranges[2].size
	};

	VkCommandBufferSubmitInfo command_buffer_submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSemaphoreSubmitInfo semaphore_wait_submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSemaphoreSubmitInfo semaphore_signal_submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSubmitInfo2 submit_infos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	
	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {

		cmdBufBegin(transfer_command_buffers[i], true);
		vkCmdCopyBuffer(transfer_command_buffers[i], global_storage_buffer_partition.buffer, frame_array.frames[i].vertex_buffer, 1, &vertex_buffer_copy);
		vkCmdCopyBuffer(transfer_command_buffers[i], global_storage_buffer_partition.buffer, frame_array.frames[i].index_buffer, 1, &index_buffer_copy);
		vkEndCommandBuffer(transfer_command_buffers[i]);

		command_buffer_submit_infos[i] = make_command_buffer_submit_info(transfer_command_buffers[i]);
		semaphore_wait_submit_infos[i] = make_timeline_semaphore_wait_submit_info(frame_array.frames[i].semaphore_render_finished, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		semaphore_signal_submit_infos[i] = make_timeline_semaphore_signal_submit_info(frame_array.frames[i].semaphore_buffers_ready, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		frame_array.frames[i].semaphore_buffers_ready.wait_counter += 1;

		submit_infos[i] = (VkSubmitInfo2){
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext = nullptr,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &semaphore_wait_submit_infos[i],
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &command_buffer_submit_infos[i],
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &semaphore_signal_submit_infos[i]
		};
	}
	vkQueueSubmit2(transfer_queue, frame_array.num_frames, submit_infos, VK_NULL_HANDLE);
}
