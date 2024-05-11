#include "frame.h"

#include "log/logging.h"
#include "render/model.h"
#include "render/render_config.h"
#include "util/allocate.h"

#include "command_buffer.h"
#include "queue.h"
#include "vertex_input.h"
#include "vulkan_manager.h"

static frame_t create_frame(physical_device_t physical_device, VkDevice device, VkCommandPool command_pool, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout) {

	frame_t frame = {
		.command_buffer = VK_NULL_HANDLE,
		.descriptor_set = VK_NULL_HANDLE,
		.semaphore_image_available = (binary_semaphore_t){ 0 },
		.semaphore_present_ready = (binary_semaphore_t){ 0 },
		.semaphore_render_finished = (timeline_semaphore_t){ 0 },
		.fence_frame_ready = VK_NULL_HANDLE,
		.semaphore_buffers_ready = (timeline_semaphore_t){ 0 },
		.vertex_buffer = VK_NULL_HANDLE,
		.index_buffer = VK_NULL_HANDLE
	};

	const VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	const VkDeviceSize render_object_vertices_size = num_render_object_slots * 4 * vertex_input_element_stride * sizeof(float);
	const VkDeviceSize area_mesh_vertices_size = num_room_layers * 32 * 32 * 4 * vertex_input_element_stride * sizeof(float);
	const VkDeviceSize vertex_buffer_size = render_object_vertices_size + area_mesh_vertices_size;
	
	const VkDeviceSize render_object_indices_size = num_render_object_slots * 6 * sizeof(index_t);
	const VkDeviceSize area_mesh_indices_size = num_room_layers * 32 * 32 * 6 * sizeof(index_t);
	const VkDeviceSize index_buffer_size = render_object_indices_size + area_mesh_indices_size;
	
	frame.semaphore_image_available = create_binary_semaphore(device);
	frame.semaphore_present_ready = create_binary_semaphore(device);
	frame.semaphore_render_finished = create_timeline_semaphore(device);
	frame.semaphore_buffers_ready = create_timeline_semaphore(device);

	vkCreateFence(device, &fence_create_info, NULL, &frame.fence_frame_ready);

	allocate_command_buffers(device, command_pool, 1, &frame.command_buffer);

	VkDescriptorSetAllocateInfo allocate_info;
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.descriptorPool = descriptor_pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &descriptor_set_layout;

	VkResult result = vkAllocateDescriptorSets(device, &allocate_info, &frame.descriptor_set);
	if (result != VK_SUCCESS) {
		logf_message(ERROR, "Descriptor set allocation failed (error code: %i).", result);
	}

	uint32_t queue_family_indices[2] = {
		*physical_device.queue_family_indices.graphics_family_ptr,
		*physical_device.queue_family_indices.transfer_family_ptr
	};

	const VkBufferCreateInfo vertex_buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.size = vertex_buffer_size,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = 2,
		.pQueueFamilyIndices = (uint32_t *)queue_family_indices
	};
	
	const VkBufferCreateInfo index_buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.size = index_buffer_size,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = 2,
		.pQueueFamilyIndices = (uint32_t *)queue_family_indices
	};
	
	vkCreateBuffer(device, &vertex_buffer_create_info, NULL, &frame.vertex_buffer);
	vkCreateBuffer(device, &index_buffer_create_info, NULL, &frame.index_buffer);

	return frame;
}

static void destroy_frame(VkDevice device, frame_t frame) {
	destroy_binary_semaphore(&frame.semaphore_image_available);
	destroy_binary_semaphore(&frame.semaphore_present_ready);
	destroy_timeline_semaphore(&frame.semaphore_render_finished);
	destroy_timeline_semaphore(&frame.semaphore_buffers_ready);
	vkDestroyFence(device, frame.fence_frame_ready, NULL);
	vkDestroyBuffer(device, frame.vertex_buffer, NULL);
	vkDestroyBuffer(device, frame.index_buffer, NULL);
}

frame_array_t create_frame_array(const frame_array_create_info_t frame_array_create_info) {

	static const uint32_t max_num_frames = 3;

	frame_array_t frame_array = { 
		.current_frame = 0,
		.num_frames = 0,
		.frames = NULL,
		.buffer_memory = VK_NULL_HANDLE,
		.device = frame_array_create_info.device
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

	if (!allocate((void **)&frame_array.frames, frame_array.num_frames, sizeof(frame_t))) {
		log_message(ERROR, "Error creating frame array: failed to allocate frame pointer-array.");
		return (frame_array_t){ 0 };
	}
	
	memory_range_t vertex_buffer_memory_ranges[3];
	memory_range_t index_buffer_memory_ranges[3];
	VkDeviceSize total_vertex_memory_size = 0;
	VkDeviceSize total_index_memory_size = 0;

	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {
		frame_array.frames[i] = create_frame(frame_array_create_info.physical_device, frame_array_create_info.device, 
			frame_array_create_info.command_pool, frame_array_create_info.descriptor_pool, frame_array_create_info.descriptor_set_layout);
		
		VkMemoryRequirements vertex_buffer_memory_requirements;
		VkMemoryRequirements index_buffer_memory_requirements;
		vkGetBufferMemoryRequirements(frame_array.device, frame_array.frames[i].vertex_buffer, &vertex_buffer_memory_requirements);
		vkGetBufferMemoryRequirements(frame_array.device, frame_array.frames[i].index_buffer, &index_buffer_memory_requirements);
		
		vertex_buffer_memory_ranges[i].offset = total_vertex_memory_size;
		vertex_buffer_memory_ranges[i].size = vertex_buffer_memory_requirements.size;
		index_buffer_memory_ranges[i].offset = total_index_memory_size;
		index_buffer_memory_ranges[i].size = index_buffer_memory_requirements.size;
		
		total_vertex_memory_size += vertex_buffer_memory_requirements.size;
		total_index_memory_size += index_buffer_memory_requirements.size;
	}
	
	VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = total_vertex_memory_size + total_index_memory_size,
		.memoryTypeIndex = memory_type_set.graphics_resources
	};
	
	vkAllocateMemory(frame_array.device, &allocate_info, NULL, &frame_array.buffer_memory);
	
	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {
		vkBindBufferMemory(frame_array.device, frame_array.frames[i].vertex_buffer, frame_array.buffer_memory, vertex_buffer_memory_ranges[i].offset);
		vkBindBufferMemory(frame_array.device, frame_array.frames[i].index_buffer, frame_array.buffer_memory, total_vertex_memory_size + index_buffer_memory_ranges[i].offset);
	}

	return frame_array;
}

bool destroy_frame_array(frame_array_t *const frame_array_ptr) {
	if (frame_array_ptr == NULL) {
		return false;
	}
	
	for (uint32_t i = 0; i < frame_array_ptr->num_frames; ++i) {
		destroy_frame(frame_array_ptr->device, frame_array_ptr->frames[i]);
	}
	deallocate((void **)&frame_array_ptr->frames);
	frame_array_ptr->num_frames = 0;
	frame_array_ptr->current_frame = 0;
	
	vkFreeMemory(frame_array_ptr->device, frame_array_ptr->buffer_memory, NULL);
	frame_array_ptr->buffer_memory = VK_NULL_HANDLE;
	frame_array_ptr->device = VK_NULL_HANDLE;
	
	return true;
}