#include "ComputeMatrices.h"

#include <string.h>

#include "log/Logger.h"
#include "render/render_config.h"

#include "../CommandBuffer.h"
#include "../ComputePipeline.h"
#include "../Descriptor.h"
#include "../VulkanManager.h"

static Pipeline computeMatricesPipeline;

static VkCommandBuffer computeMatricesCmdBuf = VK_NULL_HANDLE;

TimelineSemaphore computeMatricesSemaphore = { };

static VkFence computeMatricesFence = VK_NULL_HANDLE;

// One camera matrix, one projection matrix, and one matrix for each render object slot.
static const VkDeviceSize matrixCount = 256 + 2;

// Size in bytes of a 4x4 matrix of single-precision floating point numbers.
static const VkDeviceSize matrixSize = 4 * 4 * sizeof(float);

const VkDeviceSize matrixDataSize = matrixCount * matrixSize;

bool initComputeMatrices(const VkDevice vkDevice) {
	
	const ComputePipelineCreateInfo pipelineCreateInfo = {
		.vkDevice = vkDevice,
		.shaderFilename = makeStaticString("ComputeMatrices.spv"),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = (PushConstantRange[1]){
			{
				.shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.size = 2 * sizeof(uint32_t)
			}
		}
	};
	computeMatricesPipeline = createComputePipeline(pipelineCreateInfo);
	
	computeMatricesSemaphore = create_timeline_semaphore(vkDevice);
	
	allocCmdBufs(vkDevice, commandPoolCompute.vkCommandPool, 1, &computeMatricesCmdBuf);

	const VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	
	const VkResult fence_create_result = vkCreateFence(vkDevice, &fence_create_info, nullptr, &computeMatricesFence);
	if (fence_create_result < 0) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error initializing compute matrices pipeline: fence creation failed (result code: %i).", fence_create_result);
		deletePipeline(&computeMatricesPipeline);
		return false;
	}
	else if (fence_create_result > 0) {
		logMsg(loggerVulkan, LOG_LEVEL_WARNING, "Warning initializing compute matrices pipeline: fence creation returned with warning (result code: %i).", fence_create_result);
	}

	return true;
}

void terminateComputeMatrices(void) {
	vkDestroyFence(computeMatricesPipeline.vkDevice, computeMatricesFence, nullptr);
	destroy_timeline_semaphore(&computeMatricesSemaphore);
	deletePipeline(&computeMatricesPipeline);
}

void computeMatrices(const uint32_t transformBufferDescriptorHandle, const uint32_t matrixBufferDescriptorHandle, const float deltaTime, const ProjectionBounds projectionBounds, const Vector4F cameraPosition, const ModelTransform *const transforms) {
	if (!transforms) {
		return;
	}

	vkWaitForFences(computeMatricesPipeline.vkDevice, 1, &computeMatricesFence, VK_TRUE, UINT64_MAX);
	vkResetFences(computeMatricesPipeline.vkDevice, 1, &computeMatricesFence);

	byte_t *mapped_memory = buffer_partition_map_memory(global_uniform_buffer_partition, 0);
	if (!mapped_memory) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error computing matrices: uniform buffer memory mapping failed.");
		return;
	}
	memcpy(mapped_memory, &projectionBounds, sizeof projectionBounds);
	memcpy(mapped_memory + 24, &deltaTime, sizeof deltaTime);
	memcpy(mapped_memory + 28, &cameraPosition, sizeof cameraPosition);
	memcpy(mapped_memory + 44, transforms, numRenderObjectSlots * sizeof *transforms);
	buffer_partition_unmap_memory(global_uniform_buffer_partition);

	vkFreeCommandBuffers(computeMatricesPipeline.vkDevice, commandPoolCompute.vkCommandPool, 1, &computeMatricesCmdBuf);
	allocCmdBufs(computeMatricesPipeline.vkDevice, commandPoolCompute.vkCommandPool, 1, &computeMatricesCmdBuf);

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(computeMatricesCmdBuf, &begin_info); {

		vkCmdBindPipeline(computeMatricesCmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, computeMatricesPipeline.vkPipeline);
		
		vkCmdBindDescriptorSets(computeMatricesCmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, computeMatricesPipeline.vkPipelineLayout, 0, 1, &globalDescriptorSet, 0, nullptr);
		
		const uint32_t pushConstants[2] = { 
			transformBufferDescriptorHandle,
			matrixBufferDescriptorHandle
		};
		vkCmdPushConstants(computeMatricesCmdBuf, computeMatricesPipeline.vkPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), pushConstants);
		
		vkCmdDispatch(computeMatricesCmdBuf, 1, 1, 1);

	} vkEndCommandBuffer(computeMatricesCmdBuf);
	
	const VkCommandBufferSubmitInfo cmdBufSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = computeMatricesCmdBuf,
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
	
	vkQueueSubmit2(queueCompute, 1, &submitInfo, computeMatricesFence);
}
