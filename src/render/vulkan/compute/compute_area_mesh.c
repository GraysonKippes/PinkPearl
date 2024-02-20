#include "compute_area_mesh.h"

#include <stddef.h>
#include <string.h>

#include "log/logging.h"
#include "render/render_config.h"
#include "util/allocate.h"
#include "util/byte.h"
#include "util/offset.h"

#include "../command_buffer.h"
#include "../compute_pipeline.h"
#include "../descriptor.h"
#include "../vertex_input.h"
#include "../vulkan_manager.h"

static const descriptor_binding_t compute_area_mesh_bindings[2] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT }
};

static const descriptor_layout_t compute_area_mesh_layout = {
	.num_bindings = 2,
	.bindings = (descriptor_binding_t *)compute_area_mesh_bindings
};

static compute_pipeline_t compute_area_mesh_pipeline;

static const size_t max_area_extent_x = 32;
static const size_t max_area_extent_y = 32;
static const size_t max_num_rooms = max_area_extent_x * max_area_extent_y;
static const VkDeviceSize compute_area_mesh_uniform_size = sizeof(extent_t) + max_num_rooms * sizeof(offset_t);

void init_compute_area_mesh(const VkDevice vk_device) {
	compute_area_mesh_pipeline = create_compute_pipeline(vk_device, compute_area_mesh_layout, AREA_MESH_SHADER_NAME);
}

void terminate_compute_area_mesh(void) {
	destroy_compute_pipeline(&compute_area_mesh_pipeline);
}

void compute_area_mesh(const area_t area) {

	log_message(VERBOSE, "Generating area mesh...");

	static const VkDeviceSize uniform_offset = GLOBAL_UNIFORM_MEMORY_COMPUTE_MESH_AREA_OFFSET;
	static const VkDeviceSize uniform_size = GLOBAL_UNIFORM_MEMORY_COMPUTE_MESH_AREA_SIZE;

	static const VkDeviceSize storage_offset = GLOBAL_STORAGE_MEMORY_COMPUTE_MESH_AREA_OFFSET;
	static const VkDeviceSize storage_size = GLOBAL_STORAGE_MEMORY_COMPUTE_MESH_AREA_SIZE;

	const VkDeviceSize mesh_vertices_size = 32 * 32 * num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	const VkDeviceSize mesh_indices_size = 32 * 32 * num_indices_per_rect * sizeof(index_t);

	const uint32_t dispatch_x = area.num_rooms;
	const uint32_t dispatch_y = 4; // Number of vertices per room.
	const uint32_t dispatch_z = 1;

	offset_t *room_positions = NULL;
	if (!allocate((void **)&room_positions, area.num_rooms, sizeof(offset_t))) {
		log_message(ERROR, "Error computing area mesh: failed to allocate room position pointer-array.");
		return;
	}

	for (uint32_t i = 0; i < area.num_rooms; ++i) {
		room_positions[i] = area.rooms[i].position;
	}

	byte_t *const mapped_memory = buffer_partition_map_memory(global_uniform_buffer_partition, 2);
	memcpy(mapped_memory, &area.room_extent, sizeof area.room_extent);
	memcpy(mapped_memory + sizeof area.room_extent, room_positions, area.num_rooms * sizeof *room_positions);
	buffer_partition_unmap_memory(global_uniform_buffer_partition);
	
	const VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
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
	const VkDescriptorBufferInfo storage_buffer_info = make_descriptor_buffer_info(global_storage_buffer, storage_offset, storage_size);

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
	write_descriptor_sets[1].descriptorCount = 1;
	write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write_descriptor_sets[1].pImageInfo = NULL;
	write_descriptor_sets[1].pBufferInfo = &storage_buffer_info;
	write_descriptor_sets[1].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(compute_area_mesh_pipeline.device, 2, write_descriptor_sets, 0, NULL);

	VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, compute_command_pool, 1, &compute_command_buffer);
	begin_command_buffer(compute_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_area_mesh_pipeline.handle);
	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_area_mesh_pipeline.layout, 0, 1, &descriptor_set, 0, NULL);
	vkCmdDispatch(compute_command_buffer, dispatch_x, dispatch_y, dispatch_z);

	vkEndCommandBuffer(compute_command_buffer);
	submit_command_buffers_async(compute_queue, 1, &compute_command_buffer);
	vkFreeCommandBuffers(device, compute_command_pool, 1, &compute_command_buffer);
}
