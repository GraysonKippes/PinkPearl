#include "Buffer2.h"

#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "log/Logger.h"

struct Buffer_T {
	
	// The number of subranges or partitions of this buffer.
	int32_t subrangeCount;
	
	// If pSubrangeFlags[i] is true, then pSubranges[i] is currently locked, i.e. in use, and cannot be accessed.
	bool *pSubrangeFlags;
	
	// The array of subranges or partitions of this buffer.
	BufferSubrange *pSubranges;
	
	VkBuffer vkBuffer;
	
	VkDeviceMemory vkDeviceMemory;
	
	VkDevice vkDevice;
	
};

static const struct Buffer_T nullBufferT = {
	.subrangeCount = 0,
	.pSubrangeFlags = nullptr,
	.pSubranges = nullptr,
	.vkBuffer = VK_NULL_HANDLE,
	.vkDeviceMemory = VK_NULL_HANDLE,
	.vkDevice = VK_NULL_HANDLE
};

static bool bufferSubrangeEquals(const BufferSubrange a, const BufferSubrange b);

void createBuffer(const BufferCreateInfo bufferCreateInfo, Buffer *const pOutBuffer) {
	
	*pOutBuffer = nullptr;
	
	// Check for (some) null/invalid arguments

	if (bufferCreateInfo.queueFamilyIndexCount > 0 && !bufferCreateInfo.pQueueFamilyIndices) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating buffer: bufferCreateInfo.queueFamilyIndexCount is greater than zero, but bufferCreateInfo.pQueueFamilyIndices is null.");
		return;
	}
	
	if (bufferCreateInfo.queueFamilyIndexCount == 0 && bufferCreateInfo.pQueueFamilyIndices) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating buffer: bufferCreateInfo.queueFamilyIndexCount is zero, but bufferCreateInfo.pQueueFamilyIndices is nonnull.");
		return;
	}

	if (!bufferCreateInfo.pSubrangeSizes) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating buffer: bufferCreateInfo->pSubrangeSizes is null.");
		return;
	}

	// Allocate buffer and its arrays.
	
	Buffer buffer = malloc(sizeof(struct Buffer_T));
	if (!buffer) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating buffer: failed to allocate buffer object.");
		return;
	}
	*buffer = nullBufferT;

	buffer->pSubrangeFlags = calloc(bufferCreateInfo.subrangeCount, sizeof(bool));
	if (!buffer->pSubrangeFlags) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating buffer: failed to allocate memory for buffer->pSubrangeFlags.");
		return;
	}
	
	buffer->pSubranges = calloc(bufferCreateInfo.subrangeCount, sizeof(BufferSubrange));
	if (!buffer->pSubranges) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating buffer: failed to allocate memory for buffer->pSubranges.");
		free(buffer->pSubrangeFlags);
		return;
	}
	
	buffer->subrangeCount = bufferCreateInfo.subrangeCount;

	// Determine minimum subresource alignment requirements depending on buffer usage.
	// Default value is a generous 256 bytes.
	VkDeviceSize offsetAlignment = 256;
	switch (bufferCreateInfo.bufferType) {
		case BUFFER_TYPE_STAGING:
			offsetAlignment = bufferCreateInfo.physicalDevice.properties.limits.minMemoryMapAlignment;
			break;
		case BUFFER_TYPE_UNIFORM:
			offsetAlignment = bufferCreateInfo.physicalDevice.properties.limits.minUniformBufferOffsetAlignment;
			break;
		case BUFFER_TYPE_STORAGE:
			offsetAlignment = bufferCreateInfo.physicalDevice.properties.limits.minStorageBufferOffsetAlignment;
			break;
		case BUFFER_TYPE_DRAW_DATA:
			offsetAlignment = bufferCreateInfo.physicalDevice.properties.limits.minUniformBufferOffsetAlignment;
			break;
	}

	// Generate subranges
	VkDeviceSize totalBufferSize = 0;
	for (int32_t subrangeIndex = 0; subrangeIndex < buffer->subrangeCount; ++subrangeIndex) {
		buffer->pSubranges[subrangeIndex].index = subrangeIndex;
		buffer->pSubranges[subrangeIndex].offset = totalBufferSize;
		buffer->pSubranges[subrangeIndex].size = bufferCreateInfo.pSubrangeSizes[subrangeIndex];
		totalBufferSize += alignOffset(bufferCreateInfo.pSubrangeSizes[subrangeIndex], offsetAlignment);
	}

	VkBufferUsageFlags vkBufferUsageFlags = 0;
	switch (bufferCreateInfo.bufferType) {
		case BUFFER_TYPE_STAGING:
			vkBufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			break;
		case BUFFER_TYPE_UNIFORM:
			vkBufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
		case BUFFER_TYPE_STORAGE:
			vkBufferUsageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			break;
		case BUFFER_TYPE_DRAW_DATA:
			vkBufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			break;
	}

	VkBufferCreateInfo vkBufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = totalBufferSize,
		.usage = vkBufferUsageFlags,
		.sharingMode = bufferCreateInfo.queueFamilyIndexCount > 0 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = bufferCreateInfo.queueFamilyIndexCount,
		.pQueueFamilyIndices = bufferCreateInfo.pQueueFamilyIndices
	};

	const VkResult vkBufferCreateResult = vkCreateBuffer(bufferCreateInfo.vkDevice, &vkBufferCreateInfo, nullptr, &buffer->vkBuffer);
	if (vkBufferCreateResult < 0) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating buffer: vkBuffer creation failed (result code: %i).", vkBufferCreateResult);
		deleteBuffer(&buffer);
		return;
	} else if (vkBufferCreateResult > 0) {
		logMsg(loggerVulkan, LOG_LEVEL_WARNING, "Warning creating buffer: vkBuffer creation returned with warning (result code: %i).", vkBufferCreateResult);
	}

	const VkBufferMemoryRequirementsInfo2 vkMemoryRequirementsInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
		.pNext = nullptr,
		.buffer = buffer->vkBuffer
	};
	VkMemoryRequirements2 vkMemoryRequirements = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
		.pNext = nullptr,
		.memoryRequirements = { }
	};
	vkGetBufferMemoryRequirements2(bufferCreateInfo.vkDevice, &vkMemoryRequirementsInfo, &vkMemoryRequirements);

	uint32_t memoryTypeIndex = 0;
	switch (bufferCreateInfo.bufferType) {
		case BUFFER_TYPE_STAGING:
			memoryTypeIndex = bufferCreateInfo.memoryTypeIndexSet.resource_staging;
			break;
		case BUFFER_TYPE_UNIFORM:
			memoryTypeIndex = bufferCreateInfo.memoryTypeIndexSet.uniform_data;
			break;
		case BUFFER_TYPE_STORAGE:
			memoryTypeIndex = bufferCreateInfo.memoryTypeIndexSet.graphics_resources;
			break;
		case BUFFER_TYPE_DRAW_DATA:
			memoryTypeIndex = bufferCreateInfo.memoryTypeIndexSet.uniform_data;
			break;
	}

	const VkMemoryAllocateInfo memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = vkMemoryRequirements.memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	const VkResult vkAllocateMemoryResult = vkAllocateMemory(bufferCreateInfo.vkDevice, &memory_allocate_info, nullptr, &buffer->vkDeviceMemory);
	if (vkAllocateMemoryResult < 0) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating buffer partition: memory allocation failed (result code: %i).", vkAllocateMemoryResult);
		deleteBuffer(&buffer);
		return;
	}
	else if (vkAllocateMemoryResult > 0) {
		logMsg(loggerVulkan, LOG_LEVEL_WARNING, "Warning creating buffer partition: memory allocation returned with warning (result code: %i).", vkAllocateMemoryResult);
	}

	const VkBindBufferMemoryInfo vkBindBufferMemoryInfo = {
		.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
		.pNext = nullptr,
		.buffer = buffer->vkBuffer,
		.memory = buffer->vkDeviceMemory,
		.memoryOffset = 0
	};

	const VkResult vkBindBufferMemoryResult = vkBindBufferMemory2(bufferCreateInfo.vkDevice, 1, &vkBindBufferMemoryInfo);
	if (vkBindBufferMemoryResult < 0) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating buffer partition: memory binding failed (result code: %i).", vkBindBufferMemoryResult);
		deleteBuffer(&buffer);
		return;
	}
	else if (vkBindBufferMemoryResult > 0) {
		logMsg(loggerVulkan, LOG_LEVEL_WARNING, "Warning creating buffer partition: memory binding returned with warning (result code: %i).", vkBindBufferMemoryResult);
	}

	buffer->vkDevice = bufferCreateInfo.vkDevice;
	*pOutBuffer = buffer;
}

void deleteBuffer(Buffer *const pBuffer) {
	if (!pBuffer) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting buffer: pointer to buffer object is null.");
		return;
	}
	
	Buffer buffer = *pBuffer;
	if (!buffer) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting buffer: buffer object is null.");
		return;
	}
	
	free(buffer->pSubrangeFlags);
	free(buffer->pSubranges);
	vkDestroyBuffer(buffer->vkDevice, buffer->vkBuffer, nullptr);
	vkFreeMemory(buffer->vkDevice, buffer->vkDeviceMemory, nullptr);
	
	*buffer = nullBufferT;
	*pBuffer = nullptr;
}

void bufferBorrowSubrange(Buffer buffer, const int32_t subrangeIndex, BufferSubrange *const pOutSubrange) {
	if (!buffer) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error borrowing buffer subrange: buffer object is null.");
	}
	
	if (subrangeIndex >= buffer->subrangeCount) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error borrowing buffer subrange: subrange index (%u) is not less than buffer's subrange count (%u).", subrangeIndex, buffer->subrangeCount);
		return;
	}
	
	if (buffer->pSubrangeFlags[subrangeIndex]) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error borrowing buffer subrange: subrange %u is already being used.", subrangeIndex);
		return;
	}
	
	*pOutSubrange = buffer->pSubranges[subrangeIndex];
	buffer->pSubrangeFlags[subrangeIndex] = true;
}

void bufferReturnSubrange(Buffer buffer, BufferSubrange *const pSubrange) {
	if (!buffer) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error returning buffer subrange: buffer object is null.");
	}
	
	if (pSubrange->index >= buffer->subrangeCount) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error returning buffer subrange: subrange index (%u) is not less than buffer's subrange count (%u).", pSubrange->index, buffer->subrangeCount);
		return;
	}
	
	if (!buffer->pSubrangeFlags[pSubrange->index]) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error returning buffer subrange: subrange %u was not borrowed.", pSubrange->index);
		return;
	}
	
	if (bufferSubrangeEquals(*pSubrange, buffer->pSubranges[pSubrange->index])) {
		logMsg(loggerVulkan, LOG_LEVEL_WARNING, "Warning returning buffer subrange: subrange being returned does not match original subrange.", pSubrange->index);
		return;
	}
	
	buffer->pSubrangeFlags[pSubrange->index] = false;
	*pSubrange = (BufferSubrange){
		.index = -1,
		.offset = 0,
		.size = 0
	};
}

static bool bufferSubrangeEquals(const BufferSubrange a, const BufferSubrange b) {
	return a.index == b.index
		&& a.offset == b.offset
		&& a.size == b.size;
}