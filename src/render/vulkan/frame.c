#include "frame.h"

#include <string.h>
#include "log/Logger.h"
#include "render/render_config.h"
#include "util/allocate.h"
#include "queue.h"
#include "vertex_input.h"
#include "VulkanManager.h"

static Frame createFrame(PhysicalDevice physicalDevice, VkDevice vkDevice) {

	Frame frame = {
		.semaphore_image_available = (BinarySemaphore){ },
		.semaphore_present_ready = (BinarySemaphore){ },
		.semaphore_render_finished = (TimelineSemaphore){ },
		.fence_frame_ready = VK_NULL_HANDLE,
		.semaphore_buffers_ready = (TimelineSemaphore){ },
		.vertex_buffer = VK_NULL_HANDLE,
		.index_buffer = VK_NULL_HANDLE
	};

	const VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	vkCreateFence(vkDevice, &fence_create_info, nullptr, &frame.fence_frame_ready);
	frame.semaphore_image_available = create_binary_semaphore(vkDevice);
	frame.semaphore_present_ready = create_binary_semaphore(vkDevice);
	frame.semaphore_render_finished = create_timeline_semaphore(vkDevice);
	frame.semaphore_buffers_ready = create_timeline_semaphore(vkDevice);

	uint32_t queue_family_indices[2] = {
		*physicalDevice.queueFamilyIndices.graphics_family_ptr,
		*physicalDevice.queueFamilyIndices.transfer_family_ptr
	};

	const VkBufferCreateInfo vertex_buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = 512 * num_vertices_per_rect * vertex_input_element_stride * sizeof(float),
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = 2,
		.pQueueFamilyIndices = (uint32_t *)queue_family_indices
	};
	
	const VkBufferCreateInfo index_buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = 64 * sizeof(uint16_t),
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = 2,
		.pQueueFamilyIndices = (uint32_t *)queue_family_indices
	};
	
	vkCreateBuffer(vkDevice, &vertex_buffer_create_info, nullptr, &frame.vertex_buffer);
	vkCreateBuffer(vkDevice, &index_buffer_create_info, nullptr, &frame.index_buffer);

	return frame;
}

static void destroy_frame(VkDevice vkDevice, Frame frame) {
	destroy_binary_semaphore(&frame.semaphore_image_available);
	destroy_binary_semaphore(&frame.semaphore_present_ready);
	destroy_timeline_semaphore(&frame.semaphore_render_finished);
	destroy_timeline_semaphore(&frame.semaphore_buffers_ready);
	vkDestroyFence(vkDevice, frame.fence_frame_ready, nullptr);
	vkDestroyBuffer(vkDevice, frame.vertex_buffer, nullptr);
	vkDestroyBuffer(vkDevice, frame.index_buffer, nullptr);
}

FrameArray createFrameArray(const FrameArrayCreateInfo frameArrayCreateInfo) {

	static const uint32_t max_num_frames = 3;

	FrameArray frameArray = { 
		.current_frame = 0,
		.num_frames = 0,
		.frames = nullptr,
		.buffer_memory = VK_NULL_HANDLE,
		.device = frameArrayCreateInfo.vkDevice
	};

	if (frameArrayCreateInfo.num_frames == 0) {
		frameArray.num_frames = 1;
	}
	else if (frameArrayCreateInfo.num_frames > max_num_frames) {
		frameArray.num_frames = max_num_frames;
	}
	else {
		frameArray.num_frames = frameArrayCreateInfo.num_frames;
	}

	if (!allocate((void **)&frameArray.frames, frameArray.num_frames, sizeof(Frame))) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating frame array: failed to allocate frame pointer-array.");
		return (FrameArray){ };
	}
	
	MemoryRange vertex_buffer_memory_ranges[3];
	MemoryRange index_buffer_memory_ranges[3];
	VkDeviceSize total_vertex_memory_size = 0;
	VkDeviceSize total_index_memory_size = 0;

	for (uint32_t i = 0; i < frameArray.num_frames; ++i) {
		frameArray.frames[i] = createFrame(frameArrayCreateInfo.physical_device, frameArrayCreateInfo.vkDevice);
		
		VkMemoryRequirements vertex_buffer_memory_requirements;
		VkMemoryRequirements index_buffer_memory_requirements;
		vkGetBufferMemoryRequirements(frameArray.device, frameArray.frames[i].vertex_buffer, &vertex_buffer_memory_requirements);
		vkGetBufferMemoryRequirements(frameArray.device, frameArray.frames[i].index_buffer, &index_buffer_memory_requirements);
		
		vertex_buffer_memory_ranges[i].offset = total_vertex_memory_size;
		vertex_buffer_memory_ranges[i].size = vertex_buffer_memory_requirements.size;
		index_buffer_memory_ranges[i].offset = total_index_memory_size;
		index_buffer_memory_ranges[i].size = index_buffer_memory_requirements.size;
		
		total_vertex_memory_size += vertex_buffer_memory_requirements.size;
		total_index_memory_size += index_buffer_memory_requirements.size;
	}
	
	VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = total_vertex_memory_size + total_index_memory_size,
		.memoryTypeIndex = memory_type_set.graphics_resources
	};
	vkAllocateMemory(frameArray.device, &allocate_info, nullptr, &frameArray.buffer_memory);
	
	for (uint32_t i = 0; i < frameArray.num_frames; ++i) {
		vkBindBufferMemory(frameArray.device, frameArray.frames[i].vertex_buffer, frameArray.buffer_memory, vertex_buffer_memory_ranges[i].offset);
		vkBindBufferMemory(frameArray.device, frameArray.frames[i].index_buffer, frameArray.buffer_memory, total_vertex_memory_size + index_buffer_memory_ranges[i].offset);
	}
	
	CmdBufArray cmdBufArray = cmdBufAlloc(frameArrayCreateInfo.commandPool, 1);
	recordCommands(cmdBufArray, 0, true,
		static const VkDeviceSize indexCount = 14;
		const uint16_t indices[14] = {
			0, 1, 2, 2, 3, 0,
			0, 1, 1, 2, 2, 3, 3, 0
		};
		
		byte_t *mappedMemory = buffer_partition_map_memory(global_staging_buffer_partition, 0);
		memcpy(mappedMemory, indices, indexCount * sizeof(uint16_t));
		buffer_partition_unmap_memory(global_staging_buffer_partition);
	
		const VkBufferCopy bufCpy = {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = indexCount * sizeof(uint16_t)
		};
		for (uint32_t i = 0; i < frameArray.num_frames; ++i) {
			vkCmdCopyBuffer(cmdBuf, global_staging_buffer_partition.buffer, frameArray.frames[i].index_buffer, 1, &bufCpy);
		}
	);

	const VkCommandBufferSubmitInfo cmdBufSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = cmdBufArray.pCmdBufs[0],
		.deviceMask = 0
	};
	const VkSubmitInfo2 submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.flags = 0,
		.waitSemaphoreInfoCount = 0,
		.pWaitSemaphoreInfos = nullptr,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &cmdBufSubmitInfo,
		.signalSemaphoreInfoCount = 0,
		.pSignalSemaphoreInfos = nullptr
	};
	vkQueueSubmit2(queueGraphics, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queueGraphics);
	
	frameArray.cmdBufArray = cmdBufAlloc(frameArrayCreateInfo.commandPool, frameArrayCreateInfo.num_frames);

	return frameArray;
}

bool deleteFrameArray(FrameArray *const pFrameArray) {
	if (!pFrameArray) {
		return false;
	}
	
	for (uint32_t i = 0; i < pFrameArray->num_frames; ++i) {
		destroy_frame(pFrameArray->device, pFrameArray->frames[i]);
	}
	deallocate((void **)&pFrameArray->frames);
	cmdBufFree(&pFrameArray->cmdBufArray);
	pFrameArray->num_frames = 0;
	pFrameArray->current_frame = 0;
	
	vkFreeMemory(pFrameArray->device, pFrameArray->buffer_memory, nullptr);
	pFrameArray->buffer_memory = VK_NULL_HANDLE;
	pFrameArray->device = VK_NULL_HANDLE;
	
	return true;
}