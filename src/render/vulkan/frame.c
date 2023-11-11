#include "frame.h"

#include "command_buffer.h"
#include "queue.h"
#include "vertex_input.h"

frame_t create_frame(physical_device_t physical_device, VkDevice logical_device, VkCommandPool command_pool, descriptor_pool_t descriptor_pool) {

	frame_t frame = { 0 };

	VkSemaphoreCreateInfo semaphore_info = { 0 };
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.pNext = NULL;
	semaphore_info.flags = 0;

	VkFenceCreateInfo fence_info = { 0 };
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.pNext = NULL;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	static const VkDeviceSize matrix4F_size = 16 * sizeof(float);

	const VkDeviceSize num_elements_per_rect = num_vertices_per_rect * vertex_input_element_stride;

	static const VkDeviceSize num_room_slots = 2;

	static const VkDeviceSize max_num_models = 64;

	// General vertex buffer size
	const VkDeviceSize model_buffer_size = (max_num_models * num_elements_per_rect) * sizeof(float);

	vkCreateSemaphore(logical_device, &semaphore_info, NULL, &frame.m_semaphore_image_available);
	vkCreateSemaphore(logical_device, &semaphore_info, NULL, &frame.m_semaphore_render_finished);
	vkCreateFence(logical_device, &fence_info, NULL, &frame.m_fence_frame_ready);
	vkCreateFence(logical_device, &fence_info, NULL, &frame.m_fence_buffers_up_to_date);

	frame.m_model_update_flags = 0;

	allocate_command_buffers(logical_device, command_pool, 1, &frame.m_command_buffer);

	allocate_descriptor_sets(logical_device, descriptor_pool, 1, &frame.m_descriptor_set);

	frame.m_matrix_buffer = create_buffer(physical_device.m_handle, logical_device, max_num_models * matrix4F_size, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		queue_family_set_null);

	queue_family_set_t queue_family_set = {
		.m_num_queue_families = 2,
		.m_queue_families = (uint32_t[2]){
			*physical_device.m_queue_family_indices.m_graphics_family_ptr,
			*physical_device.m_queue_family_indices.m_transfer_family_ptr,
		}
	};

	frame.m_model_buffer = create_buffer(physical_device.m_handle, logical_device, model_buffer_size, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		queue_family_set);

	frame.m_index_buffer = create_buffer(physical_device.m_handle, logical_device, model_buffer_size, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		queue_family_set);

	return frame;
}

void destroy_frame(VkDevice logical_device, VkCommandPool command_pool, descriptor_pool_t descriptor_pool, frame_t frame) {

	vkDestroySemaphore(logical_device, frame.m_semaphore_image_available, NULL);
	vkDestroySemaphore(logical_device, frame.m_semaphore_render_finished, NULL);
	vkDestroyFence(logical_device, frame.m_fence_frame_ready, NULL);
	vkDestroyFence(logical_device, frame.m_fence_buffers_up_to_date, NULL);

	vkFreeCommandBuffers(logical_device, command_pool, 1, &frame.m_command_buffer);

	vkFreeDescriptorSets(logical_device, descriptor_pool.m_handle, 1, &frame.m_descriptor_set);

	destroy_buffer(logical_device, frame.m_matrix_buffer);
	destroy_buffer(logical_device, frame.m_model_buffer);
	destroy_buffer(logical_device, frame.m_index_buffer);
}
