#include "CommandBuffer.h"

#include <assert.h>
#include <stdlib.h>
#include "log/Logger.h"
#include "util/allocate.h"

CommandPool createCommandPool(const VkDevice vkDevice, const uint32_t queueFamilyIndex, const bool transient, const bool resetable) {
	
	CommandPool commandPool = {
		.vkCommandPool = VK_NULL_HANDLE,
		.vkDevice = VK_NULL_HANDLE,
		.transient = false,
		.resetable = false
	};
	
	VkCommandPoolCreateFlags createFlags = 0;
	if (transient) {
		createFlags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	}
	if (resetable) {
		createFlags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	}
	
	const VkCommandPoolCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = createFlags,
		.queueFamilyIndex = queueFamilyIndex
	};
	vkCreateCommandPool(vkDevice, &createInfo, nullptr, &commandPool.vkCommandPool);
	
	commandPool.vkDevice = vkDevice;
	commandPool.transient = transient;
	commandPool.resetable = resetable;
	
	return commandPool;
}

void deleteCommandPool(CommandPool *const pCommandPool) {
	if (!pCommandPool) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting command pool: null pointer(s) detected.");
		return;
	}
	
	vkDestroyCommandPool(pCommandPool->vkDevice, pCommandPool->vkCommandPool, nullptr);
	pCommandPool->vkCommandPool = VK_NULL_HANDLE;
	pCommandPool->vkDevice = VK_NULL_HANDLE;
	pCommandPool->transient = false;
	pCommandPool->resetable = false;
}

CmdBufArray cmdBufAlloc2(const CommandPool commandPool, const uint32_t count) {
	
	CmdBufArray cmdBufArray = { };
	
	cmdBufArray.pCmdBufs = heapAlloc(count, sizeof(VkCommandBuffer));
	if (!cmdBufArray.pCmdBufs) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Allocating command buffers: failed to allocate command buffer handle array (count = %u).", count);
		return (CmdBufArray){ };
	}
	cmdBufArray.count = count;
	
	const VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = commandPool.vkCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = cmdBufArray.count
	};
	const VkResult allocResult = vkAllocateCommandBuffers(commandPool.vkDevice, &allocInfo, cmdBufArray.pCmdBufs);
	if (allocResult != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Allocating command buffers: failed to allocate command buffers (error code = %i).", allocResult);
		cmdBufArray.pCmdBufs = heapFree(cmdBufArray.pCmdBufs);
		return (CmdBufArray){ };
	}
	cmdBufArray.resetable = commandPool.resetable;
	
	return cmdBufArray;
}

void cmdBufFree2(CmdBufArray *const pArr) {
	assert(pArr);
	pArr->resetable = false;
	pArr->count = 0;
	pArr->pCmdBufs = heapFree(pArr->pCmdBufs);
}

void cmdBufBegin2(const CmdBufArray arr, const uint32_t idx, const bool singleSubmit) {
	if (idx >= arr.count) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Beginning command buffer recording: command buffer index (%u) is not less than command buffer count (%u) in command buffer array %p.", idx, arr.count, arr.pCmdBufs);
		return;
	}
	
	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = singleSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0,
		.pInheritanceInfo = nullptr
	};
	const VkResult result = vkBeginCommandBuffer(arr.pCmdBufs[idx], &beginInfo);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Beginning command buffer recording: failed to begin recording (error code = %i).", result);
	}
}

void cmdBufEnd2(const CmdBufArray arr, const uint32_t idx) {
	if (idx >= arr.count) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Ending command buffer recording: command buffer index (%u) is not less than command buffer count (%u) in command buffer array %p.", idx, arr.count, arr.pCmdBufs);
		return;
	}
	
	const VkResult result = vkEndCommandBuffer(arr.pCmdBufs[idx]);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Ending command buffer recording: failed to end recording (error code = %i).", result);
	}
}

void cmdBufReset(const CmdBufArray arr, const uint32_t idx) {
	if (idx >= arr.count) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Resetting command buffer: command buffer index (%u) is not less than command buffer count (%u) in command buffer array %p.", idx, arr.count, arr.pCmdBufs);
		return;
	}
	
	if (!arr.resetable) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Resetting command buffer: command buffer array %p is not resetable.", arr.pCmdBufs);
		return;
	}
	
	const VkResult result = vkResetCommandBuffer(arr.pCmdBufs[idx], 0);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Resetting command buffer: failed to reset command buffer (error code = %i).", result);
	}
}






CommandBuffer allocateCommandBuffer(const CommandPool commandPool) {
	
	CommandBuffer commandBuffer = { 
		.vkCommandBuffer = VK_NULL_HANDLE,
		.recording = false,
		.resetable = false
	};
	
	const VkCommandBufferAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = commandPool.vkCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	vkAllocateCommandBuffers(commandPool.vkDevice, &allocateInfo, &commandBuffer.vkCommandBuffer);
	
	commandBuffer.resetable = commandPool.resetable;
	
	return commandBuffer;
}

void commandBufferBegin(CommandBuffer *const pCommandBuffer, const bool singleSubmit) {
	if (!pCommandBuffer) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error beginning command buffer recording: null pointer(s) detected.");
		return;
	} else if (pCommandBuffer->recording) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error beginning command buffer recording: command buffer already recording.");
		return;
	}
	
	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = singleSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0,
		.pInheritanceInfo = nullptr
	};
	vkBeginCommandBuffer(pCommandBuffer->vkCommandBuffer, &beginInfo);
	pCommandBuffer->recording = true;
}

void commandBufferEnd(CommandBuffer *const pCommandBuffer) {
	if (!pCommandBuffer) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error ending command buffer recording: null pointer(s) detected.");
		return;
	} else if (!pCommandBuffer->recording) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error ending command buffer recording: command buffer not already recording.");
		return;
	}
	
	vkEndCommandBuffer(pCommandBuffer->vkCommandBuffer);
	pCommandBuffer->recording = false;
}

void commandBufferBindPipeline(CommandBuffer *const pCommandBuffer, const Pipeline pipeline) {
	if (!pCommandBuffer) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error binding pipeline: pointer to command buffer object is null.");
		return;
	}
	
	VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	switch (pipeline.type) {
		case PIPELINE_TYPE_GRAPHICS:
			pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			break;
		case PIPELINE_TYPE_COMPUTE:
			pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
			break;
	}
	
	vkCmdBindPipeline(pCommandBuffer->vkCommandBuffer, pipelineBindPoint, pipeline.vkPipeline);
}

void commandBufferBindGraphicsPipeline(CommandBuffer *const pCommandBuffer, const GraphicsPipeline pipeline) {
	if (!pCommandBuffer) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error binding pipeline: pointer to command buffer object is null.");
		return;
	}
	
	vkCmdBindPipeline(pCommandBuffer->vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.vkPipeline);
}

void commandBufferReset(CommandBuffer *const pCommandBuffer) {
	if (!pCommandBuffer) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error reseting command buffer: null pointer(s) detected.");
		return;
	} else if (!pCommandBuffer->resetable) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error reseting command buffer: command buffer is not resetable.");
		return;
	}
	
	vkResetCommandBuffer(pCommandBuffer->vkCommandBuffer, 0);
}



void allocCmdBufs(VkDevice device, VkCommandPool command_pool, uint32_t num_buffers, VkCommandBuffer *command_buffers) {

	VkCommandBufferAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandPool = command_pool;
	allocate_info.commandBufferCount = num_buffers;

	// TODO - error handling
	vkAllocateCommandBuffers(device, &allocate_info, command_buffers);
}

void cmdBufBegin(const VkCommandBuffer cmdBuf, const bool singleSubmit) {
	const VkCommandBufferBeginInfo cmdBufBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = singleSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0,
		.pInheritanceInfo = nullptr
	};
	vkBeginCommandBuffer(cmdBuf, &cmdBufBeginInfo);
}

VkCommandBufferSubmitInfo make_command_buffer_submit_info(const VkCommandBuffer command_buffer) {
	return (VkCommandBufferSubmitInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = command_buffer,
		.deviceMask = 0
	};
}

void submit_command_buffers_async(VkQueue queue, uint32_t num_command_buffers, VkCommandBuffer *command_buffers) {

	VkSubmitInfo submit_info = { 0 };
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.pWaitDstStageMask = nullptr;
	submit_info.commandBufferCount = num_command_buffers;
	submit_info.pCommandBuffers = command_buffers;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = nullptr;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = nullptr;

	vkQueueSubmit(queue, 1, &submit_info, nullptr);
	vkQueueWaitIdle(queue);
}
