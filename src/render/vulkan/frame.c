#include "frame.h"

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

	static const VkDeviceSize num_elements_per_rect = 4 * 8;

	static const VkDeviceSize num_room_slots = 2;

	static const VkDeviceSize max_num_models = 64;

	// General vertex buffer size		Rooms - 2 slots						// Entity models
	const VkDeviceSize model_buffer_size = ((32 * 20 * num_elements_per_rect * num_room_slots) + (max_num_models * num_elements_per_rect)) * sizeof(float);

	vkCreateSemaphore(logical_device, &semaphore_info, NULL, &frame.m_semaphore_image_available);
	vkCreateSemaphore(logical_device, &semaphore_info, NULL, &frame.m_semaphore_render_finished);
	vkCreateFence(logical_device, &fence_info, NULL, &frame.m_fence_frame_ready);

	vkCreateFence(logical_device, &fence_info, NULL, &frame.m_fence_buffers_up_to_date);

	frame.m_model_update_flags = 0;

	allocate_command_buffers(logical_device, command_pool, 1, &frame.m_command_buffer);

	allocate_descriptor_sets(logical_device, descriptor_pool, 1, &frame.m_descriptor_set);

	frame.m_matrix_buffer = create_buffer(physical_device.m_handle, logical_device, max_num_models * matrix4F_size, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	frame.m_model_buffer = create_shared_buffer(physical_device, logical_device, model_buffer_size, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	frame.m_index_buffer = create_shared_buffer(physical_device, logical_device, model_buffer_size, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	return frame;
}
