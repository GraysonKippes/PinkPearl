#include "compute_matrices.h"

#include <string.h>

#include "log/logging.h"
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

static compute_pipeline_t compute_matrices_pipeline;

static VkCommandBuffer compute_matrices_command_buffer = VK_NULL_HANDLE;
static VkDescriptorSet compute_matrices_descriptor_set = VK_NULL_HANDLE;
VkSemaphore compute_matrices_semaphore = VK_NULL_HANDLE;
static VkFence compute_matrices_fence = VK_NULL_HANDLE;


// One camera matrix, one projection matrix, and one matrix for each render object slot.
static const VkDeviceSize num_matrices = NUM_RENDER_OBJECT_SLOTS + 2;
	
// Size in bytes of a 4x4 matrix of single-precision floating point numbers.
static const VkDeviceSize matrix_size = 16 * sizeof(float);

const VkDeviceSize matrix_data_size = num_matrices * matrix_size;



bool init_compute_matrices(const VkDevice vk_device) {

	compute_matrices_pipeline = create_compute_pipeline(vk_device, compute_matrices_layout, COMPUTE_MATRICES_SHADER_NAME);
	allocate_command_buffers(vk_device, compute_command_pool, 1, &compute_matrices_command_buffer);

	const VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = compute_matrices_pipeline.descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &compute_matrices_pipeline.descriptor_set_layout
	};

	const VkResult descriptor_set_allocate_result = vkAllocateDescriptorSets(vk_device, &descriptor_set_allocate_info, &compute_matrices_descriptor_set);
	if (descriptor_set_allocate_result < 0) {
		logf_message(ERROR, "Error initializing compute matrices pipeline: descriptor set allocation failed (result code: %i).", descriptor_set_allocate_result);
		destroy_compute_pipeline(&compute_matrices_pipeline);
		return false;
	}
	else if (descriptor_set_allocate_result > 0) {
		logf_message(WARNING, "Warning initializing compute matrices pipeline: descriptor set allocation returned with warning (result code: %i).", descriptor_set_allocate_result);
	}


	const VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

	const VkResult semaphore_create_result = vkCreateSemaphore(vk_device, &semaphore_create_info, NULL, &compute_matrices_semaphore);
	if (semaphore_create_result < 0) {
		logf_message(ERROR, "Error initializing compute matrices pipeline: semaphore creation failed (result code: %i).", semaphore_create_result);
		destroy_compute_pipeline(&compute_matrices_pipeline);
		return false;
	}
	else if (semaphore_create_result > 0) {
		logf_message(WARNING, "Warning initializing compute matrices pipeline: semaphore creation returned with warning (result code: %i).", semaphore_create_result);
	}

	const VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	
	const VkResult fence_create_result = vkCreateFence(vk_device, &fence_create_info, NULL, &compute_matrices_fence);
	if (fence_create_result < 0) {
		logf_message(ERROR, "Error initializing compute matrices pipeline: fence creation failed (result code: %i).", fence_create_result);
		vkDestroySemaphore(vk_device, compute_matrices_semaphore, NULL);
		destroy_compute_pipeline(&compute_matrices_pipeline);
		return false;
	}
	else if (fence_create_result > 0) {
		logf_message(WARNING, "Warning initializing compute matrices pipeline: fence creation returned with warning (result code: %i).", fence_create_result);
	}

	return true;
}

void terminate_compute_matrices(void) {
	vkDestroyFence(compute_matrices_pipeline.device, compute_matrices_fence, NULL);
	vkDestroySemaphore(compute_matrices_pipeline.device, compute_matrices_semaphore, NULL);
	destroy_compute_pipeline(&compute_matrices_pipeline);
}

void compute_matrices(const float delta_time, const projection_bounds_t projection_bounds, const vector3F_t camera_position, const render_position_t *const positions) {

	vkWaitForFences(compute_matrices_pipeline.device, 1, &compute_matrices_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(compute_matrices_pipeline.device, 1, &compute_matrices_fence);

	byte_t *mapped_memory = buffer_partition_map_memory(global_uniform_buffer_partition, 0);
	if (mapped_memory == NULL) {
		log_message(ERROR, "Error computing matrices: uniform buffer memory mapping failed.");
		return;
	}

	memcpy(mapped_memory, &projection_bounds, sizeof projection_bounds);
	memcpy(mapped_memory + 32, &delta_time, sizeof delta_time);
	memcpy(mapped_memory + 48, &camera_position, sizeof camera_position);

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		static const VkDeviceSize starting_offset = 64;
		memcpy(mapped_memory + starting_offset + (i * 32), &positions[i].position, sizeof(vector3F_t));
		memcpy(mapped_memory + starting_offset + (i * 32) + 16, &positions[i].previous_position, sizeof(vector3F_t));
	}

	buffer_partition_unmap_memory(global_uniform_buffer_partition);

	const VkDescriptorBufferInfo uniform_buffer_info = buffer_partition_descriptor_info(global_uniform_buffer_partition, 0);
	const VkDescriptorBufferInfo storage_buffer_info = buffer_partition_descriptor_info(global_storage_buffer_partition, 0);

	VkWriteDescriptorSet descriptor_writes[2] = { { 0 } };

	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = compute_matrices_descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &uniform_buffer_info;
	descriptor_writes[0].pImageInfo = NULL;
	descriptor_writes[0].pTexelBufferView = NULL;

	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = compute_matrices_descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pBufferInfo = &storage_buffer_info;
	descriptor_writes[1].pImageInfo = NULL;
	descriptor_writes[1].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device, 2, descriptor_writes, 0, NULL);

	vkFreeCommandBuffers(compute_matrices_pipeline.device, compute_command_pool, 1, &compute_matrices_command_buffer);
	allocate_command_buffers(compute_matrices_pipeline.device, compute_command_pool, 1, &compute_matrices_command_buffer);

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};

	vkBeginCommandBuffer(compute_matrices_command_buffer, &begin_info);

	vkCmdBindPipeline(compute_matrices_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_matrices_pipeline.handle);
	vkCmdBindDescriptorSets(compute_matrices_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_matrices_pipeline.layout, 0, 1, &compute_matrices_descriptor_set, 0, NULL);
	vkCmdDispatch(compute_matrices_command_buffer, 1, 1, 1);

	vkEndCommandBuffer(compute_matrices_command_buffer);

	const VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = NULL,
		.commandBufferCount = 1,
		.pCommandBuffers = &compute_matrices_command_buffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &compute_matrices_semaphore
	};

	vkQueueSubmit(compute_queue, 1, &submit_info, compute_matrices_fence);
}
