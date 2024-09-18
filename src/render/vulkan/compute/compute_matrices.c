#include "compute_matrices.h"

#include <string.h>

#include "log/Logger.h"
#include "render/render_config.h"

#include "../CommandBuffer.h"
#include "../ComputePipeline.h"
#include "../descriptor.h"
#include "../VulkanManager.h"

static const DescriptorBinding compute_matrices_bindings[2] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT }
};

static const DescriptorSetLayout compute_matrices_layout = {
	.num_bindings = 2,
	.bindings = (DescriptorBinding *)compute_matrices_bindings
};

static Pipeline compute_matrices_pipeline;

static VkCommandBuffer compute_matrices_command_buffer = VK_NULL_HANDLE;

static VkDescriptorSet compute_matrices_descriptor_set = VK_NULL_HANDLE;

TimelineSemaphore computeMatricesSemaphore = { };

static VkFence compute_matrices_fence = VK_NULL_HANDLE;

// One camera matrix, one projection matrix, and one matrix for each render object slot.
static const VkDeviceSize num_matrices = 516;

// Size in bytes of a 4x4 matrix of single-precision floating point numbers.
static const VkDeviceSize matrix_size = 4 * 4 * sizeof(float);

const VkDeviceSize matrix_data_size = num_matrices * matrix_size;

bool init_compute_matrices(const VkDevice vkDevice) {

	compute_matrices_pipeline = createComputePipeline(vkDevice, compute_matrices_layout, COMPUTE_MATRICES_SHADER_NAME);
	
	computeMatricesSemaphore = create_timeline_semaphore(vkDevice);
	
	allocCmdBufs(vkDevice, commandPoolCompute.vkCommandPool, 1, &compute_matrices_command_buffer);

	const VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = compute_matrices_pipeline.vkDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &compute_matrices_pipeline.vkDescriptorSetLayout
	};

	const VkResult descriptor_set_allocate_result = vkAllocateDescriptorSets(vkDevice, &descriptor_set_allocate_info, &compute_matrices_descriptor_set);
	if (descriptor_set_allocate_result < 0) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error initializing compute matrices pipeline: descriptor set allocation failed (result code: %i).", descriptor_set_allocate_result);
		deletePipeline(&compute_matrices_pipeline);
		return false;
	}
	else if (descriptor_set_allocate_result > 0) {
		logMsg(loggerVulkan, LOG_LEVEL_WARNING, "Warning initializing compute matrices pipeline: descriptor set allocation returned with warning (result code: %i).", descriptor_set_allocate_result);
	}

	const VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	
	const VkResult fence_create_result = vkCreateFence(vkDevice, &fence_create_info, nullptr, &compute_matrices_fence);
	if (fence_create_result < 0) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error initializing compute matrices pipeline: fence creation failed (result code: %i).", fence_create_result);
		//vkDestroySemaphore(vkDevice, compute_matrices_semaphore, nullptr);
		deletePipeline(&compute_matrices_pipeline);
		return false;
	}
	else if (fence_create_result > 0) {
		logMsg(loggerVulkan, LOG_LEVEL_WARNING, "Warning initializing compute matrices pipeline: fence creation returned with warning (result code: %i).", fence_create_result);
	}

	return true;
}

void terminate_compute_matrices(void) {
	vkDestroyFence(compute_matrices_pipeline.vkDevice, compute_matrices_fence, nullptr);
	//vkDestroySemaphore(compute_matrices_pipeline.vkDevice, compute_matrices_semaphore, nullptr);
	destroy_timeline_semaphore(&computeMatricesSemaphore);
	deletePipeline(&compute_matrices_pipeline);
}

void computeMatrices(const VkDeviceSize bufferOffset, const float deltaTime, const ProjectionBounds projectionBounds, const Vector4F cameraPosition, const ModelTransform *const transforms) {
	if (!transforms) {
		return;
	}

	vkWaitForFences(compute_matrices_pipeline.vkDevice, 1, &compute_matrices_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(compute_matrices_pipeline.vkDevice, 1, &compute_matrices_fence);

	byte_t *mapped_memory = buffer_partition_map_memory(global_uniform_buffer_partition, 0);
	if (!mapped_memory) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error computing matrices: uniform buffer memory mapping failed.");
		return;
	}
	memcpy(mapped_memory, &projectionBounds, sizeof projectionBounds);
	memcpy(mapped_memory + 32, &deltaTime, sizeof deltaTime);
	memcpy(mapped_memory + 48, &cameraPosition, sizeof cameraPosition);
	memcpy(mapped_memory + 64, transforms, numRenderObjectSlots * sizeof *transforms);
	buffer_partition_unmap_memory(global_uniform_buffer_partition);

	const VkDescriptorBufferInfo uniform_buffer_info = buffer_partition_descriptor_info(global_uniform_buffer_partition, 0);
	//const VkDescriptorBufferInfo storage_buffer_info = buffer_partition_descriptor_info(global_storage_buffer_partition, 0);

	const VkDescriptorBufferInfo storage_buffer_info = {
		.buffer = global_storage_buffer_partition.buffer,
		.offset = bufferOffset,
		.range = 16512
	};

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

	vkFreeCommandBuffers(compute_matrices_pipeline.vkDevice, commandPoolCompute.vkCommandPool, 1, &compute_matrices_command_buffer);
	allocCmdBufs(compute_matrices_pipeline.vkDevice, commandPoolCompute.vkCommandPool, 1, &compute_matrices_command_buffer);

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(compute_matrices_command_buffer, &begin_info);

	vkCmdBindPipeline(compute_matrices_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_matrices_pipeline.vkPipeline);
	vkCmdBindDescriptorSets(compute_matrices_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_matrices_pipeline.vkPipelineLayout, 0, 1, &compute_matrices_descriptor_set, 0, nullptr);
	vkCmdDispatch(compute_matrices_command_buffer, 1, 1, 1);

	vkEndCommandBuffer(compute_matrices_command_buffer);
	
	const VkCommandBufferSubmitInfo cmdBufSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = compute_matrices_command_buffer,
		.deviceMask = 0
	};
	
	const VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo = make_timeline_semaphore_signal_submit_info(computeMatricesSemaphore, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
	computeMatricesSemaphore.wait_counter += 1;
	
	const VkSubmitInfo2 submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.waitSemaphoreInfoCount = 0,
		.pWaitSemaphoreInfos = nullptr,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &cmdBufSubmitInfo,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = &signalSemaphoreSubmitInfo
	};
	
	vkQueueSubmit2(queueCompute, 1, &submitInfo, compute_matrices_fence);
}
