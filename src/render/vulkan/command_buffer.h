#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <stdint.h>

#include <vulkan/vulkan.h>

extern const VkCommandPoolCreateFlags default_command_pool_flags;

extern const VkCommandPoolCreateFlags transfer_command_pool_flags;

void create_command_pool(VkDevice device, VkCommandPoolCreateFlags flags, uint32_t queue_family_index, VkCommandPool *command_pool_ptr);

void allocate_command_buffers(VkDevice device, VkCommandPool command_pool, uint32_t num_buffers, VkCommandBuffer *command_buffers);

void begin_command_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage);

void begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, VkClearValue *clear_value);

// Submits command buffers to the specified queue, without synchronization from either semaphores or fences.
// Useful for single-use command buffers such as those used for transfer operations.
void submit_command_buffers_async(VkQueue queue, uint32_t num_command_buffers, VkCommandBuffer *command_buffers);

#endif // COMMAND_BUFFER_H
