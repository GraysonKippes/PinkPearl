#include "compute_matrices.h"

#include <string.h>

#include "render/render_config.h"

#include "../command_buffer.h"
#include "../compute_pipeline.h"
#include "../descriptor.h"
#include "../vulkan_manager.h"



static const descriptor_binding_t compute_matrices_bindings[2] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT }
};

static const descriptor_layout_t compute_matrices_layout = {
	.num_bindings = 2,
	.bindings = (descriptor_binding_t *)compute_matrices_bindings
};

static compute_pipeline_t compute_pipeline_matrices;



// One camera matrix, one projection matrix, and one matrix for each render object slot.
static const VkDeviceSize num_matrices = NUM_RENDER_OBJECT_SLOTS + 2;
	
// Size in bytes of a 4x4 matrix of single-precision floating point numbers.
static const VkDeviceSize matrix_size = 16 * sizeof(float);

const VkDeviceSize matrix_data_size = num_matrices * matrix_size;



void init_compute_matrices(void) {
	compute_pipeline_matrices = create_compute_pipeline(device, compute_matrices_layout, "compute_matrices.spv");
}

void terminate_compute_matrices(void) {
	destroy_compute_pipeline(device, compute_pipeline_matrices);
}

void compute_matrices(float delta_time, projection_bounds_t projection_bounds, render_position_t camera_position, render_position_t *positions) {

	static const VkDeviceSize uniform_data_size = 1024;

	memcpy(global_uniform_mapped_memory, &delta_time, sizeof delta_time);
	memcpy(global_uniform_mapped_memory + 4, &projection_bounds, sizeof projection_bounds);
	memcpy(global_uniform_mapped_memory + 32, &camera_position, sizeof camera_position);
	memcpy(global_uniform_mapped_memory + 64, positions, num_render_object_slots * sizeof(render_position_t));

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
	matrix_buffer_info.buffer = global_storage_buffer;
	matrix_buffer_info.offset = 0;
	matrix_buffer_info.range = matrix_data_size;

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
