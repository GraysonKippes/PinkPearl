#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <stdint.h>
#include <vulkan/vulkan.h>
#include "descriptor.h"
#include "GraphicsPipeline.h"
#include "Pipeline.h"

// TODO: design and implement a data-oriented command buffer system.
// Command pools can be wrapped into objects because there will only be a small limited set of them, and only one will be used at a time.
// Command buffers should NOT be wrapped into objects, but arrays of command buffers can be packed into structs along with command buffer count.

typedef struct CommandPool {
	
	VkCommandPool vkCommandPool;
	VkDevice vkDevice;
	
	bool transient;
	bool resetable;
	
} CommandPool;

// Creates a new command pool in the specified queue family.
CommandPool createCommandPool(const VkDevice vkDevice, const uint32_t queueFamilyIndex, const bool transient, const bool resetable);

// Deletes the command pool and resets its objects to null values.
void deleteCommandPool(CommandPool *const pCommandPool);

typedef struct CmdBufArray {
	bool resetable;
	uint32_t count;
	VkCommandBuffer *pCmdBufs;
	VkCommandPool vkCommandPool;
	VkDevice vkDevice;
} CmdBufArray;

// Allocates an array of command buffers and returns them in a struct.
// commandPool: the command pool in which the command buffers will be allocated.
// count: the number of command buffers to allocate.
CmdBufArray cmdBufAlloc(const CommandPool commandPool, const uint32_t count);

void cmdBufFree(CmdBufArray *const pArr);

void cmdBufBegin(const CmdBufArray arr, const uint32_t idx, const bool singleSubmit);

void cmdBufEnd(const CmdBufArray arr, const uint32_t idx);

void cmdBufReset(const CmdBufArray arr, const uint32_t idx);

VkCommandBufferSubmitInfo make_command_buffer_submit_info(const VkCommandBuffer command_buffer);

// TODO: remove.
// Submits command buffers to the specified queue, without synchronization from either semaphores or fences.
// Useful for single-use command buffers such as those used for transfer operations.
void submit_command_buffers_async(VkQueue queue, uint32_t num_command_buffers, VkCommandBuffer *command_buffers);

#endif // COMMAND_BUFFER_H
