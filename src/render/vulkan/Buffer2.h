#ifndef BUFFER_2_H
#define BUFFER_2_H

#include <stdint.h>

#include "buffer.h"
#include "memory.h"
#include "physical_device.h"

typedef struct Buffer_T *Buffer;

typedef struct BufferSubrange {
	
	// The buffer from which this subrange was borrowed.
	Buffer owner;
	
	// The index of this subrange inside of the owner buffer.
	int32_t index;
	
	// The offset into the buffer's memory.
	VkDeviceSize offset;
	
	// The size of this subrange within the buffer's memory.
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

// Creates a new buffer with the properties specified in bufferCreateInfo.
void createBuffer(const BufferCreateInfo bufferCreateInfo, Buffer *const pOutBuffer);

// Deletes the specified buffer and resets the handle to null.
void deleteBuffer(Buffer *const pBuffer);

VkBuffer bufferGetVkBuffer(const Buffer buffer);

// "Borrows" a subrange or partition from the buffer, locking that subrange until it is returned.
void bufferBorrowSubrange(Buffer buffer, const int32_t subrangeIndex, BufferSubrange *const pOutSubrange);

// "Returns" a previously borrowed subrange to its owner, unlocking that subrange.
void bufferReturnSubrange(BufferSubrange *const pSubrange);

void bufferHostTransfer(const BufferSubrange subrange, const VkDeviceSize dataOffset, const VkDeviceSize dataSize, const void *const pData);

VkDescriptorBufferInfo makeDescriptorBufferInfo2(const BufferSubrange subrange);

#endif	// BUFFER_2_H