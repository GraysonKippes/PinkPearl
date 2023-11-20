#include "frame.h"

#include "command_buffer.h"
#include "queue.h"
#include "vertex_input.h"

frame_t create_frame(physical_device_t physical_device, VkDevice device, VkCommandPool command_pool, descriptor_pool_t descriptor_pool) {

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

	vkCreateSemaphore(device, &semaphore_info, NULL, &frame.semaphore_image_available);
	vkCreateSemaphore(device, &semaphore_info, NULL, &frame.semaphore_render_finished);
	vkCreateSemaphore(device, &semaphore_info, NULL, &frame.semaphore_compute_matrices_finished);

	vkCreateFence(device, &fence_info, NULL, &frame.fence_frame_ready);
	vkCreateFence(device, &fence_info, NULL, &frame.fence_buffers_up_to_date);

	frame.model_update_flags = 0;

	allocate_command_buffers(device, command_pool, 1, &frame.command_buffer);

	allocate_descriptor_sets(device, descriptor_pool, 1, &frame.descriptor_set);

	queue_family_set_t queue_family_set = {
		.num_queue_families = 2,
		.queue_families = (uint32_t[2]){
			*physical_device.queue_family_indices.graphics_family_ptr,
			*physical_device.queue_family_indices.transfer_family_ptr,
		}
	};

	frame.model_buffer = create_buffer(physical_device.handle, device, model_buffer_size, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		queue_family_set);

	frame.index_buffer = create_buffer(physical_device.handle, device, model_buffer_size, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		queue_family_set);

	return frame;
}

void destroy_frame(VkDevice device, VkCommandPool command_pool, descriptor_pool_t descriptor_pool, frame_t frame) {

	vkDestroySemaphore(device, frame.semaphore_image_available, NULL);
	vkDestroySemaphore(device, frame.semaphore_render_finished, NULL);
	vkDestroySemaphore(device, frame.semaphore_compute_matrices_finished, NULL);

	vkDestroyFence(device, frame.fence_frame_ready, NULL);
	vkDestroyFence(device, frame.fence_buffers_up_to_date, NULL);

	vkFreeCommandBuffers(device, command_pool, 1, &frame.command_buffer);

	vkFreeDescriptorSets(device, descriptor_pool.handle, 1, &frame.descriptor_set);

	destroy_buffer(&frame.model_buffer);
	destroy_buffer(&frame.index_buffer);
}
