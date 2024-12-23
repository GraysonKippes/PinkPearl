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

CmdBufArray cmdBufAlloc(const CommandPool commandPool, const uint32_t count) {
	
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
	cmdBufArray.vkCommandPool = commandPool.vkCommandPool;
	cmdBufArray.vkDevice = commandPool.vkDevice;
	
	return cmdBufArray;
}

void cmdBufFree(CmdBufArray *const pArr) {
	assert(pArr);
	/*if (pArr->resetable) {
		for (uint32_t i = 0; i < pArr->count; ++i) {
			const VkResult result = vkResetCommandBuffer(pArr->pCmdBufs[i], 0);
			if (result != VK_SUCCESS) {
				logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Freeing command buffer array: failed to reset command buffer %u in command buffer array %p (error code = %i).", i, pArr->pCmdBufs, result);
			}
		}
	}*/
	vkFreeCommandBuffers(pArr->vkDevice, pArr->vkCommandPool, pArr->count, pArr->pCmdBufs);
	pArr->resetable = false;
	pArr->count = 0;
	pArr->pCmdBufs = heapFree(pArr->pCmdBufs);
}

void cmdBufBegin(const CmdBufArray arr, const uint32_t idx, const bool singleSubmit) {
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

void cmdBufEnd(const CmdBufArray arr, const uint32_t idx) {
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
