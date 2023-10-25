#include "vulkan_manager.h"

#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "debug.h"
#include "util.h"
#include "log/logging.h"
#include "glfw/glfw_manager.h"

#include "buffer.h"
#include "command_buffer.h"
#include "compute_pipeline.h"
#include "descriptor.h"
#include "frame.h"
#include "graphics_pipeline.h"
#include "logical_device.h"
#include "physical_device.h"
#include "shader.h"
#include "swapchain.h"
#include "vertex_input.h"
#include "vulkan_instance.h"

/* -- Vulkan Objects -- */

static VkInstance vulkan_instance = VK_NULL_HANDLE;
static VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

static VkSurfaceKHR surface = VK_NULL_HANDLE;

static physical_device_t physical_device;

static VkDevice logical_device = VK_NULL_HANDLE;

static swapchain_t swapchain;

static graphics_pipeline_t graphics_pipeline;

/* -- Compute -- */

#define NUM_COMPUTE_PIPELINES 1

static VkPipeline compute_pipelines[NUM_COMPUTE_PIPELINES];

static VkPipelineLayout compute_pipeline_layouts[NUM_COMPUTE_PIPELINES];

static VkPipeline compute_pipeline_matrices;
static VkPipelineLayout compute_pipeline_layout_matrices;

/* -- Queues -- */

static VkQueue graphics_queue;
static VkQueue present_queue;
static VkQueue transfer_queue;
static VkQueue compute_queue;

/* -- Command Pools -- */

static VkCommandPool render_command_pool;
static VkCommandPool transfer_command_pool;
static VkCommandPool compute_command_pool;



/* -- Render Objects -- */

#define MAX_FRAMES_IN_FLIGHT	2

static frame_t frames[MAX_FRAMES_IN_FLIGHT];

static size_t current_frame = 0;

#define FRAME (frames[current_frame])

static descriptor_pool_t graphics_descriptor_pool = { 0 };

// Used for staging model data into the model buffers.
static buffer_t model_staging_buffer;
static buffer_t index_staging_buffer;

// TODO - collate storage buffers as much as possible.

// Buffers for compute_matrices shader.
static buffer_t compute_matrices_buffer;
static buffer_t render_positions_buffer;
static buffer_t matrix_buffer;

static void create_buffers(void);

/* -- Function Definitions -- */

void create_vulkan_objects(void) {

	create_vulkan_instance(&vulkan_instance);
	if (debug_enabled)
		setup_debug_messenger(vulkan_instance, &debug_messenger);

	{
		log_message(INFO, "Creating window surface...");

		VkResult result = glfwCreateWindowSurface(vulkan_instance, get_application_window(), NULL, &surface);
		if (result != VK_SUCCESS) {
			logf_message(FATAL, "Window surface creation failed. (Error code: %i)", result);
			exit(1);
		}
	}

	physical_device = select_physical_device(vulkan_instance, surface);

	create_logical_device(physical_device, &logical_device);

	log_message(VERBOSE, "Retrieving device queues...");

	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_graphics_family_ptr, 0, &graphics_queue);
	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_present_family_ptr, 0, &present_queue);
	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_transfer_family_ptr, 0, &transfer_queue);
	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_compute_family_ptr, 0, &compute_queue);

	create_command_pool(logical_device, default_command_pool_flags, *physical_device.m_queue_family_indices.m_graphics_family_ptr, &render_command_pool);
	create_command_pool(logical_device, transfer_command_pool_flags, *physical_device.m_queue_family_indices.m_transfer_family_ptr, &transfer_command_pool);
	create_command_pool(logical_device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, *physical_device.m_queue_family_indices.m_compute_family_ptr, &compute_command_pool);



	swapchain = create_swapchain(get_application_window(), surface, physical_device, logical_device, VK_NULL_HANDLE);



	VkShaderModule vertex_shader;
	create_shader_module(logical_device, "vertex_models.spv", &vertex_shader);

	VkShaderModule fragment_shader;
	create_shader_module(logical_device, "fragment_test.spv", &fragment_shader);

	VkDescriptorSetLayout graphics_pipeline_layout = VK_NULL_HANDLE;
	create_descriptor_set_layout(logical_device, graphics_descriptor_layout, &graphics_pipeline_layout);

	graphics_pipeline = create_graphics_pipeline(logical_device, swapchain, graphics_pipeline_layout, vertex_shader, fragment_shader);

	vkDestroyShaderModule(logical_device, vertex_shader, NULL);
	vkDestroyShaderModule(logical_device, fragment_shader, NULL);



	create_framebuffers(logical_device, graphics_pipeline.m_render_pass, &swapchain);



	const uint32_t num_compute_shaders = NUM_COMPUTE_PIPELINES;

	compute_shader_t compute_matrices;
	create_shader_module(logical_device, "compute_matrices.spv", &compute_matrices.m_module);
	create_descriptor_set_layout(logical_device, compute_matrices_layout, &compute_matrices.m_descriptor_set_layout);

	create_compute_pipelines(logical_device, compute_pipelines, compute_pipeline_layouts, num_compute_shaders, compute_matrices);
	compute_pipeline_matrices = compute_pipelines[0];
	compute_pipeline_layout_matrices = compute_pipeline_layouts[0];

	vkDestroyShaderModule(logical_device, compute_matrices.m_module, NULL);



	create_descriptor_pool(logical_device, MAX_FRAMES_IN_FLIGHT, graphics_descriptor_layout, &graphics_descriptor_pool.m_handle);
	create_descriptor_set_layout(logical_device, graphics_descriptor_layout, &graphics_descriptor_pool.m_layout);

	log_message(VERBOSE, "Creating frame objects...");

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		frames[i] = create_frame(physical_device, logical_device, render_command_pool, graphics_descriptor_pool);
	}

	create_buffers();
}

static void create_buffers(void) {


	static const VkDeviceSize num_elements_per_rect = 4 * 8;

	static const VkDeviceSize num_room_slots = 2;

	static const VkDeviceSize max_num_models = 64;

	// General vertex buffer size
	const VkDeviceSize model_buffer_size = max_num_models * num_elements_per_rect * sizeof(float);

	model_staging_buffer = create_buffer(physical_device.m_handle, logical_device, model_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	const VkDeviceSize index_buffer_size = max_num_models * 6 * sizeof(index_t);

	index_staging_buffer = create_buffer(physical_device.m_handle, logical_device, index_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	compute_matrices_buffer = create_buffer(physical_device.m_handle, logical_device, 68, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	VkDeviceSize render_positions_buffer_size = 64 * sizeof(render_position_t);

	render_positions_buffer = create_buffer(physical_device.m_handle, logical_device, render_positions_buffer_size, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	VkDeviceSize matrix_buffer_size = 64 * 16 * sizeof(float);

	matrix_buffer = create_buffer(physical_device.m_handle, logical_device, matrix_buffer_size, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void destroy_vulkan_objects(void) {

	vkDeviceWaitIdle(logical_device);

	destroy_buffer(logical_device, compute_matrices_buffer);
	destroy_buffer(logical_device, render_positions_buffer);
	destroy_buffer(logical_device, matrix_buffer);
	destroy_buffer(logical_device, index_staging_buffer);
	destroy_buffer(logical_device, model_staging_buffer);
}

// Copy the model data to the appropriate staging buffers, and signal the render engine to transfer that data to the buffers on the GPU.
void stage_model_data(render_handle_t handle, model_t model) {

	logf_message(VERBOSE, "Staging model data; handle = %i", handle);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkResetFences(logical_device, 1, &frames[i].m_fence_buffers_up_to_date);
		frames[i].m_model_update_flags |= (1LL << (uint64_t)handle);
	}

	VkDeviceSize model_size = 4 * 8 * sizeof(float);
	VkDeviceSize model_offset = handle * model_size;

	void *model_staging_data;
	vkMapMemory(logical_device, model_staging_buffer.m_memory, model_offset, model_size, 0, &model_staging_data);
	memcpy(model_staging_data, model.m_vertices, model_size);
	vkUnmapMemory(logical_device, model_staging_buffer.m_memory);

	VkDeviceSize indices_size = 6 * sizeof(index_t);
	VkDeviceSize indices_offset = handle * indices_size;

	index_t rect_indices[6] = {
		0, 1, 2,
		2, 3, 0
	};
	for (size_t i = 0; i < 6; ++i)
		rect_indices[i] += (handle * 4) + 5120;

	void *index_staging_data;
	vkMapMemory(logical_device, index_staging_buffer.m_memory, indices_offset, indices_size, 0, &index_staging_data);
	memcpy(index_staging_data, rect_indices, indices_size);
	vkUnmapMemory(logical_device, index_staging_buffer.m_memory);
}

// Dispatches a work load to the compute_matrices shader, computing a transformation matrix for each render object.
void compute_matrices(uint32_t num_inputs, float delta_time, render_position_t camera_position, projection_bounds_t projection_bounds, render_position_t *positions) {

	typedef uint8_t byte_t;

	byte_t *compute_matrices_data;
	vkMapMemory(logical_device, compute_matrices_buffer.m_memory, 0, 68, 0, &compute_matrices_data);

	memcpy(compute_matrices_data, &num_inputs, sizeof num_inputs);
	memcpy(compute_matrices_data + 4, &delta_time, sizeof delta_time);
	memcpy(compute_matrices_data + 16, &camera_position, sizeof camera_position);
	memcpy(compute_matrices_data + 44, &projection_bounds, sizeof projection_bounds);

	vkUnmapMemory(logical_device, compute_matrices_buffer.m_memory);

	descriptor_pool_t descriptor_pool;
	create_descriptor_pool(logical_device, 1, compute_matrices_layout, &descriptor_pool.m_handle);
	create_descriptor_set_layout(logical_device, compute_matrices_layout, &descriptor_pool.m_layout);

	VkDescriptorSet descriptor_set;
	allocate_descriptor_sets(logical_device, descriptor_pool, 1, &descriptor_set);

	VkDescriptorBufferInfo compute_matrices_buffer_info = { 0 };
	compute_matrices_buffer_info.buffer = compute_matrices_buffer.m_handle;
	compute_matrices_buffer_info.offset = 0;
	compute_matrices_buffer_info.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo render_positions_buffer_info = { 0 };
	render_positions_buffer_info.buffer = render_positions_buffer.m_handle;
	render_positions_buffer_info.offset = 0;
	render_positions_buffer_info.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo matrix_buffer_info = { 0 };
	matrix_buffer_info.buffer = matrix_buffer.m_handle;
	matrix_buffer_info.offset = 0;
	matrix_buffer_info.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet descriptor_writes[2] = { { 0 } };

	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &compute_matrices_buffer_info;
	descriptor_writes[0].pImageInfo = NULL;
	descriptor_writes[0].pTexelBufferView = NULL;

	VkDescriptorBufferInfo storage_buffer_infos[2] = { render_positions_buffer_info, matrix_buffer_info };

	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[1].descriptorCount = 2;
	descriptor_writes[1].pBufferInfo = storage_buffer_infos;
	descriptor_writes[1].pImageInfo = NULL;
	descriptor_writes[1].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(logical_device, 2, descriptor_writes, 0, NULL);



	VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(logical_device, compute_command_pool, 1, &compute_command_buffer);

	VkCommandBufferBeginInfo begin_info = { 0 };
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(compute_command_buffer, &begin_info);

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_matrices);
	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout_matrices, 0, 1, &descriptor_set, 0, NULL);

	vkCmdDispatch(compute_command_buffer, num_inputs, 1, 1);

	vkEndCommandBuffer(compute_command_buffer);

	VkSubmitInfo submit_info = { 0 };
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &compute_command_buffer;

	vkQueueSubmit(compute_queue, 1, &submit_info, NULL);
}

// Send the drawing commands to the GPU to draw the frame.
void draw_frame(double delta_time) {

	uint32_t image_index = 0;

	vkWaitForFences(logical_device, 1, &FRAME.m_fence_frame_ready, VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(logical_device, swapchain.m_handle, UINT64_MAX, FRAME.m_semaphore_image_available, VK_NULL_HANDLE, &image_index);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		// TODO - error handling
	}

	// If the buffers of this frame are not up-to-date, update them.
	if (vkGetFenceStatus(logical_device, FRAME.m_fence_buffers_up_to_date) == VK_NOT_READY) {
	
		VkCommandBuffer transfer_command_buffers[2] = { 0 };
		allocate_command_buffers(logical_device, transfer_command_pool, 2, transfer_command_buffers);

		VkCommandBuffer transfer_command_buffer = transfer_command_buffers[0];
		VkCommandBuffer index_transfer_command_buffer = transfer_command_buffers[1];

		VkCommandBufferBeginInfo begin_info = { 0 };
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext = NULL;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = NULL;

		vkBeginCommandBuffer(transfer_command_buffer, &begin_info);
		vkBeginCommandBuffer(index_transfer_command_buffer, &begin_info);

		VkBufferCopy copy_regions[64] = { { 0 } };
		VkBufferCopy index_copy_regions[64] = { { 0 } };

		// The number of models needing to be updated.
		uint32_t num_pending_models = 0;

		for (uint64_t i = 0; i < 64; ++i) {
			if (FRAME.m_model_update_flags & (1LL << i)) {

				copy_regions[num_pending_models].srcOffset = (i * 4 * 8) * sizeof(float);
				copy_regions[num_pending_models].dstOffset = (32 * 20 * 4 * 8 * 2) * sizeof(float) + copy_regions[num_pending_models].srcOffset;
				copy_regions[num_pending_models].size = 4 * 8 * sizeof(float);

				index_copy_regions[num_pending_models].srcOffset = (i * 6) * sizeof(float);
				index_copy_regions[num_pending_models].dstOffset = (32 * 20 * 6 * 2) * sizeof(index_t) + index_copy_regions[num_pending_models].srcOffset;
				index_copy_regions[num_pending_models].size = 6 * sizeof(index_t);

				++num_pending_models;
			}
		}

		FRAME.m_model_update_flags = 0;

		vkCmdCopyBuffer(transfer_command_buffer, model_staging_buffer.m_handle, FRAME.m_model_buffer.m_handle, num_pending_models, copy_regions);
		vkCmdCopyBuffer(index_transfer_command_buffer, index_staging_buffer.m_handle, FRAME.m_index_buffer.m_handle, num_pending_models, index_copy_regions);

		vkEndCommandBuffer(index_transfer_command_buffer);
		vkEndCommandBuffer(transfer_command_buffer);

		VkSubmitInfo submit_info = { 0 };
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext = NULL;
		submit_info.commandBufferCount = 2;
		submit_info.pCommandBuffers = transfer_command_buffers;

		vkQueueSubmit(transfer_queue, 1, &submit_info, FRAME.m_fence_buffers_up_to_date);

		vkQueueWaitIdle(transfer_queue);

		vkFreeCommandBuffers(logical_device, transfer_command_pool, 2, transfer_command_buffers);
	}


	// Compute matrices
	// Signal a semaphore when the entire batch in the compute queue is done being executed.

	render_position_t camera_position = { 0 };
	projection_bounds_t projection_bounds = { .left = -12.0F, .right = 12.0F, .top = 7.5F, .bottom = -7.5F, .near = 15.0F, .far = -15.0F };

	render_position_t pos0 = { { 1, 3, 2 }, { 1, 3, 2 } };

	render_position_t positions[1] = { pos0 };

	compute_matrices(1, (float)delta_time, camera_position, projection_bounds, positions);



	// Wait until the buffers are fully up-to-date.
	vkQueueWaitIdle(compute_queue);
	
	vkWaitForFences(logical_device, 1, &FRAME.m_fence_buffers_up_to_date, VK_TRUE, UINT64_MAX);

	vkResetFences(logical_device, 1, &FRAME.m_fence_frame_ready);

	vkResetCommandBuffer(FRAME.m_command_buffer, 0);

	VkDescriptorBufferInfo matrix_buffer_info = { 0 };
	matrix_buffer_info.buffer = matrix_buffer.m_handle;
	matrix_buffer_info.offset = 0;
	matrix_buffer_info.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet descriptor_write = { 0 };
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = FRAME.m_descriptor_set;
	descriptor_write.dstBinding = 0;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_write.descriptorCount = 1;
	descriptor_write.pBufferInfo = &matrix_buffer_info;
	descriptor_write.pImageInfo = NULL;
	descriptor_write.pTexelBufferView = NULL;
	
	vkUpdateDescriptorSets(logical_device, 1, &descriptor_write, 0, NULL);



	VkCommandBufferBeginInfo begin_info = { 0 };
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(FRAME.m_command_buffer, &begin_info);

	VkClearValue clear_color = {{{ 0.0F, 0.001F, 0.02F, 1.0F }}};

	VkRenderPassBeginInfo render_pass_info = { 0 };
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.pNext = NULL;
	render_pass_info.renderPass = graphics_pipeline.m_render_pass;
	render_pass_info.framebuffer = swapchain.m_framebuffers[image_index];
	render_pass_info.renderArea.offset.x = 0;
	render_pass_info.renderArea.offset.y = 0;
	render_pass_info.renderArea.extent = swapchain.m_extent;
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;

	vkCmdBeginRenderPass(FRAME.m_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(FRAME.m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.m_handle);

	vkCmdBindDescriptorSets(FRAME.m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.m_layout, 0, 1, &FRAME.m_descriptor_set, 0, NULL);

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(FRAME.m_command_buffer, 0, 1, &FRAME.m_model_buffer.m_handle, offsets);

	vkCmdBindIndexBuffer(FRAME.m_command_buffer, FRAME.m_index_buffer.m_handle, 0, VK_INDEX_TYPE_UINT16);

	const uint32_t model_slot = 0;
	vkCmdPushConstants(FRAME.m_command_buffer, graphics_pipeline.m_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, (sizeof model_slot), &model_slot);

	vkCmdDrawIndexed(FRAME.m_command_buffer, 6, 1, 7680, 0, 0);

	vkCmdEndRenderPass(FRAME.m_command_buffer);

	vkEndCommandBuffer(FRAME.m_command_buffer);



	VkPipelineStageFlags wait_stages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submit_info = { 0 };
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &FRAME.m_command_buffer;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &FRAME.m_semaphore_image_available;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &FRAME.m_semaphore_render_finished;

	if (vkQueueSubmit(graphics_queue, 1, &submit_info, FRAME.m_fence_frame_ready) != VK_SUCCESS) {
		// TODO - error handling
	}



	VkPresentInfoKHR present_info = { 0 };
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &FRAME.m_semaphore_render_finished;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain.m_handle;
	present_info.pImageIndices = &image_index;
	present_info.pResults = NULL;

	vkQueuePresentKHR(present_queue, &present_info);

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}
