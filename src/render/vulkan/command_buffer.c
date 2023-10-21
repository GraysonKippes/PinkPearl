#include "command_buffer.h"

#include "log/logging.h"

const VkCommandPoolCreateFlags default_command_pool_flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

const VkCommandPoolCreateFlags transfer_command_pool_flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // TODO - this pool may not need the reset flag.


void create_command_pool(VkDevice logical_device, VkCommandPoolCreateFlags flags, uint32_t queue_family_index, VkCommandPool *command_pool_ptr) {
	
	logf_message(INFO, "Creating command pool in queue family %i...", queue_family_index);

	VkCommandPoolCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = flags;
	create_info.queueFamilyIndex = queue_family_index;

	// TODO - error handling
	VkResult result = vkCreateCommandPool(logical_device, &create_info, NULL, command_pool_ptr);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Command pool creation failed. (Error code: %i)", result);
	}
}

void allocate_command_buffers(VkDevice logical_device, VkCommandPool command_pool, uint32_t num_buffers, VkCommandBuffer *command_buffers) {

	VkCommandBufferAllocateInfo allocate_info;
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandPool = command_pool;
	allocate_info.commandBufferCount = num_buffers;

	// TODO - error handling
	vkAllocateCommandBuffers(logical_device, &allocate_info, command_buffers);
}

void begin_command_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage) {

	VkCommandBufferBeginInfo begin_info;
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = usage;
	begin_info.pInheritanceInfo = NULL;
	
	// TODO - error handling
	vkBeginCommandBuffer(command_buffer, &begin_info);
}

void begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, VkClearValue *clear_value) {

	VkRenderPassBeginInfo render_pass_info;
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
