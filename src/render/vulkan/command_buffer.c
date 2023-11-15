#include "command_buffer.h"

#include "log/logging.h"

const VkCommandPoolCreateFlags default_command_pool_flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

const VkCommandPoolCreateFlags transfer_command_pool_flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // TODO - this pool may not need the reset flag.


void create_command_pool(VkDevice device, VkCommandPoolCreateFlags flags, uint32_t queue_family_index, VkCommandPool *command_pool_ptr) {
	
	logf_message(VERBOSE, "Creating command pool in queue family %i...", queue_family_index);

	VkCommandPoolCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = flags;
	create_info.queueFamilyIndex = queue_family_index;

	// TODO - error handling
	VkResult result = vkCreateCommandPool(device, &create_info, NULL, command_pool_ptr);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Command pool creation failed. (Error code: %i)", result);
	}
}

void allocate_command_buffers(VkDevice device, VkCommandPool command_pool, uint32_t num_buffers, VkCommandBuffer *command_buffers) {

	VkCommandBufferAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandPool = command_pool;
	allocate_info.commandBufferCount = num_buffers;

	// TODO - error handling
	vkAllocateCommandBuffers(device, &allocate_info, command_buffers);
}

void begin_command_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage) {

	VkCommandBufferBeginInfo begin_info = { 0 };
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = usage;
	begin_info.pInheritanceInfo = NULL;
	
	vkBeginCommandBuffer(command_buffer, &begin_info);
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

void submit_command_buffers_async(VkQueue queue, uint32_t num_command_buffers, VkCommandBuffer *command_buffers) {

	VkSubmitInfo submit_info = { 0 };
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.pWaitDstStageMask = NULL;
	submit_info.commandBufferCount = num_command_buffers;
	submit_info.pCommandBuffers = command_buffers;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = NULL;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = NULL;

	vkQueueSubmit(queue, 1, &submit_info, NULL);
	vkQueueWaitIdle(queue);
}
