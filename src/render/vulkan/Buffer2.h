#ifndef BUFFER_2_H
#define BUFFER_2_H

#include <stdint.h>

#include "buffer.h"
#include "memory.h"
#include "physical_device.h"

typedef struct Buffer_T *Buffer;

typedef struct BufferSubrange {
	
	int32_t index;
	
	VkDeviceSize offset;
	
	VkDeviceSize size;
	
} BufferSubrange;

typedef struct BufferCreateInfo {
	
	PhysicalDevice physicalDevice;
	VkDevice vkDevice;

	BufferType bufferType;
	MemoryTypeIndexSet memoryTypeIndexSet;

	uint32_t queueFamilyIndexCount;
	uint32_t *pQueueFamilyIndices;

	uint32_t subrangeCount;
	VkDeviceSize *pSubrangeSizes;
	
} BufferCreateInfo;

void createBuffer(const BufferCreateInfo bufferCreateInfo, Buffer *const pOutBuffer);

void deleteBuffer(Buffer *const pBuffer);

#endif	// BUFFER_2_H