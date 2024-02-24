#include "frame.h"

#include "log/logging.h"
#include "render/model.h"
#include "render/render_config.h"
#include "util/allocate.h"

#include "command_buffer.h"
#include "queue.h"
#include "vertex_input.h"

frame_t create_frame(physical_device_t physical_device, VkDevice device, VkCommandPool command_pool, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout) {

	frame_t frame = { 0 };

	const VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

	const VkSemaphoreTypeCreateInfo timeline_semaphore_type_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.pNext = NULL,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = 0
	};

	const VkSemaphoreCreateInfo timeline_semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &timeline_semaphore_type_create_info,
		.flags = 0
	};

	const VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	const VkDeviceSize num_elements_per_rect = num_vertices_per_rect * vertex_input_element_stride;
	const VkDeviceSize model_buffer_size = (num_render_object_slots * num_elements_per_rect) * sizeof(float);
	const VkDeviceSize index_buffer_size = (num_render_object_slots * num_indices_per_rect) * sizeof(index_t);

	vkCreateSemaphore(device, &semaphore_create_info, NULL, &frame.semaphore_image_available);
	
	frame.semaphore_present_ready = create_binary_semaphore(device);
	frame.semaphore_render_finished = create_timeline_semaphore(device);
	frame.semaphore_buffers_ready = create_timeline_semaphore(device);

	vkCreateFence(device, &fence_create_info, NULL, &frame.fence_frame_ready);

	frame.model_update_flags = 0;

	allocate_command_buffers(device, command_pool, 1, &frame.command_buffer);

	VkDescriptorSetAllocateInfo allocate_info;
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.descriptorPool = descriptor_pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &descriptor_set_layout;

	VkResult result = vkAllocateDescriptorSets(device, &allocate_info, &frame.descriptor_set);
	if (result != VK_SUCCESS) {
		logf_message(ERROR, "Descriptor set allocation failed. (Error code: %i)", result);
	}

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

	frame.index_buffer = create_buffer(physical_device.handle, device, index_buffer_size, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		queue_family_set);

	return frame;
}

void destroy_frame(VkDevice device, frame_t frame) {

	vkDestroySemaphore(device, frame.semaphore_image_available, NULL);
	destroy_binary_semaphore(&frame.semaphore_present_ready);
	destroy_timeline_semaphore(&frame.semaphore_render_finished);
	destroy_timeline_semaphore(&frame.semaphore_buffers_ready);
	vkDestroyFence(device, frame.fence_frame_ready, NULL);

	destroy_buffer(&frame.model_buffer);
	destroy_buffer(&frame.index_buffer);
}

frame_array_t create_frame_array(const frame_array_create_info_t frame_array_create_info) {

	static const uint32_t max_num_frames = 3;

	frame_array_t frame_array = { 
		.num_frames = 0,
		.frames = NULL,
		.buffer_memory = VK_NULL_HANDLE,
		.device = VK_NULL_HANDLE
	};

	if (frame_array_create_info.num_frames == 0) {
		frame_array.num_frames = 1;
	}
	else if (frame_array_create_info.num_frames > max_num_frames) {
		frame_array.num_frames = max_num_frames;
	}
	else {
		frame_array.num_frames = frame_array_create_info.num_frames;
	}

	if (!allocate((void **)frame_array.frames, frame_array.num_frames, sizeof(frame_t))) {
		log_message(ERROR, "Error creating frame array: failed to allocate frame pointer-array.");
		return (frame_array_t){ 0 };
	}

	static const VkDeviceSize vertex_buffer_size = 5120 + 81920;
	static const VkDeviceSize index_buffer_size = 768 + 12288;

	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {
		
		const VkSemaphoreCreateInfo semaphore_create_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0
		};

		const VkFenceCreateInfo fence_create_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = NULL,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

	}

	return frame_array;
}
