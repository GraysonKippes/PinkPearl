#include "command_buffer.h"

#include <stdlib.h>

#include "log/logging.h"

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
		logMsg(LOG_LEVEL_ERROR, "Error deleting command pool: null pointer(s) detected.");
		return;
	}
	
	vkDestroyCommandPool(pCommandPool->vkDevice, pCommandPool->vkCommandPool, nullptr);
	pCommandPool->vkCommandPool = VK_NULL_HANDLE;
	pCommandPool->vkDevice = VK_NULL_HANDLE;
	pCommandPool->transient = false;
	pCommandPool->resetable = false;
}

CommandBuffer allocateCommandBuffer(const CommandPool commandPool) {
	
	CommandBuffer commandBuffer = { 
		.vkCommandBuffer = VK_NULL_HANDLE,
		.recording = false,
		.resetable = false,
		.boundDescriptorSetCount = 0,
		.ppBoundDescriptorSets = nullptr
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
	commandBuffer.ppBoundDescriptorSets = malloc(8 * sizeof(DescriptorSet *));
	
	return commandBuffer;
}

void commandBufferBegin(CommandBuffer *const pCommandBuffer, const bool singleSubmit) {
	if (!pCommandBuffer) {
		logMsg(LOG_LEVEL_ERROR, "Error beginning command buffer recording: null pointer(s) detected.");
		return;
	} else if (pCommandBuffer->recording) {
		logMsg(LOG_LEVEL_ERROR, "Error beginning command buffer recording: command buffer already recording.");
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
		logMsg(LOG_LEVEL_ERROR, "Error ending command buffer recording: null pointer(s) detected.");
		return;
	} else if (!pCommandBuffer->recording) {
		logMsg(LOG_LEVEL_ERROR, "Error ending command buffer recording: command buffer not already recording.");
		return;
	}
	
	vkEndCommandBuffer(pCommandBuffer->vkCommandBuffer);
	pCommandBuffer->recording = false;
}

void commandBufferBindDescriptorSet(CommandBuffer *const pCommandBuffer, DescriptorSet *const pDescriptorSet, const Pipeline pipeline) {
	if (!pCommandBuffer || !pDescriptorSet) {
		logMsg(LOG_LEVEL_ERROR, "Error binding descriptor set: null pointer(s) detected.");
		return;
	} else if (!pCommandBuffer->recording) {
		logMsg(LOG_LEVEL_ERROR, "Error binding descriptor set: command buffer not currently recording.");
		return;
	} else if (pCommandBuffer->boundDescriptorSetCount >= 8) {
		logMsg(LOG_LEVEL_ERROR, "Error binding descriptor set: maximum number of descriptor sets already bound.");
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
	
	vkCmdBindDescriptorSets(pCommandBuffer->vkCommandBuffer, pipelineBindPoint, pipeline.vkPipelineLayout, 0, 1, &pDescriptorSet->vkDescriptorSet, 0, nullptr);
	pCommandBuffer->ppBoundDescriptorSets[pCommandBuffer->boundDescriptorSetCount] = pDescriptorSet;
	pCommandBuffer->boundDescriptorSetCount += 1;
	pDescriptorSet->bound = true;
}

void commandBufferReset(CommandBuffer *const pCommandBuffer) {
	if (!pCommandBuffer) {
		logMsg(LOG_LEVEL_ERROR, "Error reseting command buffer: null pointer(s) detected.");
		return;
	} else if (!pCommandBuffer->resetable) {
		logMsg(LOG_LEVEL_ERROR, "Error reseting command buffer: command buffer is not resetable.");
		return;
	}
	
	vkResetCommandBuffer(pCommandBuffer->vkCommandBuffer, 0);
	
	for (uint32_t i = 0; i < pCommandBuffer->boundDescriptorSetCount; ++i) {
		const DescriptorSet descriptorSet = *pCommandBuffer->ppBoundDescriptorSets[i];
		vkUpdateDescriptorSets(descriptorSet.vkDevice, descriptorSet.pendingWriteCount, descriptorSet.pPendingWrites, 0, nullptr);
		pCommandBuffer->ppBoundDescriptorSets[i] = nullptr;
	}
	pCommandBuffer->boundDescriptorSetCount = 0;
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

void begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, VkClearValue *clear_value) {

	VkRenderPassBeginInfo render_pass_info = { 0 };
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = render_pass;
	render_pass_info.framebuffer = framebuffer;
	render_pass_info.renderArea.offset.x = 0;
	render_pass_info.renderArea.offset.y = 0;
	render_pass_info.renderArea.extent = extent;
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = clear_value;

	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
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
