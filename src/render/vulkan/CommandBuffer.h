#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "descriptor.h"
#include "Pipeline.h"



typedef struct CommandPool {
	
	VkCommandPool vkCommandPool;
	VkDevice vkDevice;
	
	bool transient;
	bool resetable;
	
} CommandPool;

typedef struct CommandBuffer {
	
	VkCommandBuffer vkCommandBuffer;
	
	bool recording;
	bool resetable;
	
	uint32_t boundDescriptorSetCount;
	DescriptorSet **ppBoundDescriptorSets;
	
} CommandBuffer;

// Creates a new command pool in the specified queue family.
CommandPool createCommandPool(const VkDevice vkDevice, const uint32_t queueFamilyIndex, const bool transient, const bool resetable);

// Deletes the command pool and resets its objects to null values.
void deleteCommandPool(CommandPool *const pCommandPool);

CommandBuffer allocateCommandBuffer(const CommandPool commandPool);

void commandBufferBegin(CommandBuffer *const pCommandBuffer, const bool singleSubmit);

void commandBufferEnd(CommandBuffer *const pCommandBuffer);

void commandBufferBindDescriptorSet(CommandBuffer *const pCommandBuffer, DescriptorSet *const pDescriptorSet, const Pipeline pipeline);

void commandBufferReset(CommandBuffer *const pCommandBuffer);



void allocCmdBufs(const VkDevice vkDevice, const VkCommandPool commandPool, const uint32_t numBuffers, VkCommandBuffer *pCommandBuffers);

void cmdBufBegin(const VkCommandBuffer cmdBuf, const bool singleSubmit);

void begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, VkClearValue *clear_value);

VkCommandBufferSubmitInfo make_command_buffer_submit_info(const VkCommandBuffer command_buffer);

// Submits command buffers to the specified queue, without synchronization from either semaphores or fences.
// Useful for single-use command buffers such as those used for transfer operations.
void submit_command_buffers_async(VkQueue queue, uint32_t num_command_buffers, VkCommandBuffer *command_buffers);

#endif // COMMAND_BUFFER_H
