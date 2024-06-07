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

static ComputePipeline compute_matrices_pipeline;

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
		.pNext = nullptr,
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
		.pNext = nullptr,
		.flags = 0
	};

	const VkResult semaphore_create_result = vkCreateSemaphore(vk_device, &semaphore_create_info, nullptr, &compute_matrices_semaphore);
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
		.pNext = nullptr,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	
	const VkResult fence_create_result = vkCreateFence(vk_device, &fence_create_info, nullptr, &compute_matrices_fence);
	if (fence_create_result < 0) {
		logf_message(ERROR, "Error initializing compute matrices pipeline: fence creation failed (result code: %i).", fence_create_result);
		vkDestroySemaphore(vk_device, compute_matrices_semaphore, nullptr);
		destroy_compute_pipeline(&compute_matrices_pipeline);
		return false;
	}
	else if (fence_create_result > 0) {
		logf_message(WARNING, "Warning initializing compute matrices pipeline: fence creation returned with warning (result code: %i).", fence_create_result);
	}

	return true;
}

void terminate_compute_matrices(void) {
	vkDestroyFence(compute_matrices_pipeline.device, compute_matrices_fence, nullptr);
	vkDestroySemaphore(compute_matrices_pipeline.device, compute_matrices_semaphore, nullptr);
	destroy_compute_pipeline(&compute_matrices_pipeline);
}

void computeMatrices(const float deltaTime, const projection_bounds_t projectionBounds, const Vector4F cameraPosition, const RenderTransform *const transforms) {
	if (transforms == nullptr) {
		return;
	}

	vkWaitForFences(compute_matrices_pipeline.device, 1, &compute_matrices_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(compute_matrices_pipeline.device, 1, &compute_matrices_fence);

	byte_t *mapped_memory = buffer_partition_map_memory(global_uniform_buffer_partition, 0);
	if (mapped_memory == nullptr) {
		log_message(ERROR, "Error computing matrices: uniform buffer memory mapping failed.");
		return;
	}
	memcpy(mapped_memory, &projectionBounds, sizeof projectionBounds);
	memcpy(mapped_memory + 32, &deltaTime, sizeof deltaTime);
	memcpy(mapped_memory + 48, &cameraPosition, sizeof cameraPosition);
	memcpy(mapped_memory + 64, transforms, numRenderObjectSlots * sizeof *transforms);
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
	descriptor_writes[0].pImageInfo = nullptr;
	descriptor_writes[0].pTexelBufferView = nullptr;

	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = compute_matrices_descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pBufferInfo = &storage_buffer_info;
	descriptor_writes[1].pImageInfo = nullptr;
	descriptor_writes[1].pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(device, 2, descriptor_writes, 0, nullptr);

	vkFreeCommandBuffers(compute_matrices_pipeline.device, compute_command_pool, 1, &compute_matrices_command_buffer);
	allocate_command_buffers(compute_matrices_pipeline.device, compute_command_pool, 1, &compute_matrices_command_buffer);

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(compute_matrices_command_buffer, &begin_info);

	vkCmdBindPipeline(compute_matrices_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_matrices_pipeline.handle);
	vkCmdBindDescriptorSets(compute_matrices_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_matrices_pipeline.layout, 0, 1, &compute_matrices_descriptor_set, 0, nullptr);
	vkCmdDispatch(compute_matrices_command_buffer, 1, 1, 1);

	vkEndCommandBuffer(compute_matrices_command_buffer);

	const VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &compute_matrices_command_buffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &compute_matrices_semaphore
	};

	vkQueueSubmit(compute_queue, 1, &submit_info, compute_matrices_fence);
}
