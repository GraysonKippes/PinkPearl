#include "vulkan_render.h"

#include <stdlib.h>
#include <string.h>

#include "log/logging.h"

#include "command_buffer.h"
#include "vertex_input.h"
#include "vulkan_manager.h"



static VkClearValue clear_color;

// TODO - collate storage buffers as much as possible, and move to vulkan_manager.

// Buffers for compute_matrices shader.
static buffer_t matrix_buffer;		// Storage - GPU only

// Objects for room texture compute shader.
static buffer_t image_staging_buffer;	// Staging - CPU-to-GPU data flow
static image_t tilemap_texture;		// Storage - GPU only
static image_t room_texture_storage;	// Storage - GPU only
static image_t room_texture;		// Graphics - GPU only



void create_vulkan_render_buffers(void) {

	log_message(VERBOSE, "Creating Vulkan render buffers...");

	VkDeviceSize staging_buffer_size = 4096;

	image_staging_buffer = create_buffer(physical_device.handle, device, staging_buffer_size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			queue_family_set_null);

	static const VkDeviceSize max_num_models = 64;

	VkDeviceSize matrix_buffer_size = 64 * 16 * sizeof(float);

	matrix_buffer = create_buffer(physical_device.handle, device, matrix_buffer_size, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			queue_family_set_null);
}

void create_vulkan_render_images(void) {

	log_message(VERBOSE, "Creating Vulkan render images...");

	// Apparently the staging buffer can't be created if the image data is loaded first...
	image_data_t image_data = load_image_data("../resources/assets/textures/tilemap/dungeon2.png");
	
	map_data_to_whole_buffer(device, image_staging_buffer, image_data.data);

	create_sampler(physical_device, device, &sampler_default);

	image_dimensions_t tilemap_texture_dimensions = { (uint32_t)image_data.width, (uint32_t)image_data.height };

	queue_family_set_t queue_family_set_0 = {
		.num_queue_families = 2,
		.queue_families = (uint32_t[2]){
			*physical_device.queue_family_indices.transfer_family_ptr,
			*physical_device.queue_family_indices.compute_family_ptr,
		}
	};

	tilemap_texture = create_image(physical_device.handle, device, 
			tilemap_texture_dimensions, 
			VK_FORMAT_R8G8B8A8_UINT, 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			queue_family_set_0);

	transition_image_layout(graphics_queue, render_command_pool, &tilemap_texture, IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_TRANSFER_DST);

	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, transfer_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkBufferImageCopy2 copy_region = make_buffer_image_copy((VkOffset2D){ 0 }, (VkExtent2D){ 32, 32 });

	VkCopyBufferToImageInfo2 copy_info = { 0 };
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
	copy_info.pNext = NULL;
	copy_info.srcBuffer = image_staging_buffer.handle;
	copy_info.dstImage = tilemap_texture.handle;
	copy_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	copy_info.regionCount = 1;
	copy_info.pRegions = &copy_region;

	vkCmdCopyBufferToImage2(transfer_command_buffer, &copy_info);

	vkEndCommandBuffer(transfer_command_buffer);
	submit_command_buffers_async(transfer_queue, 1, &transfer_command_buffer);
	vkFreeCommandBuffers(device, transfer_command_pool, 1, &transfer_command_buffer);

	destroy_buffer(&image_staging_buffer);
	free_image_data(image_data);

	// Transition tilemap texture layout so that it can be read by the compute shader.
	transition_image_layout(graphics_queue, render_command_pool, &tilemap_texture, IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_GENERAL);
}

void create_vulkan_render_objects(void) {

	log_message(VERBOSE, "Creating Vulkan render objects...");

	create_vulkan_render_buffers();
	create_vulkan_render_images();
}

void destroy_vulkan_render_objects(void) {

	vkDeviceWaitIdle(device);

	destroy_buffer(&matrix_buffer);
	
	destroy_image(tilemap_texture);
	destroy_image(room_texture_storage);
}

// Copy the model data to the appropriate staging buffers, and signal the render engine to transfer that data to the buffers on the GPU.
void stage_model_data(render_handle_t handle, model_t model) {

	// Vertex Data

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkResetFences(device, 1, &frames[i].fence_buffers_up_to_date);
		frames[i].model_update_flags |= (1LL << (uint64_t)handle);
	}

	VkDeviceSize model_size = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	VkDeviceSize model_offset = handle * model_size;

	memcpy((global_staging_mapped_memory + model_offset), model.vertices, model_size);

	// Index Data

	VkDeviceSize base_offset = model_size * 64;
	VkDeviceSize indices_size = num_indices_per_rect * sizeof(index_t);
	VkDeviceSize indices_offset = handle * indices_size;

	index_t rect_indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	for (size_t i = 0; i < num_indices_per_rect; ++i) {
		rect_indices[i] += handle * 4;
	}

	memcpy((global_staging_mapped_memory + base_offset + indices_offset), rect_indices, indices_size);
}

void transfer_model_data(void) {

	VkCommandBuffer transfer_command_buffers[2] = { 0 };
	allocate_command_buffers(device, transfer_command_pool, 2, transfer_command_buffers);

	VkCommandBuffer transfer_command_buffer = transfer_command_buffers[0];
	VkCommandBuffer index_transfer_command_buffer = transfer_command_buffers[1];

	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	begin_command_buffer(index_transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkBufferCopy vertex_copy_regions[64] = { { 0 } };
	VkBufferCopy index_copy_regions[64] = { { 0 } };

	// The number of models needing to be updated.
	uint32_t num_pending_models = 0;

	const VkDeviceSize copy_size_vertices = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	const VkDeviceSize copy_size_indices = num_indices_per_rect * sizeof(index_t);

	const VkDeviceSize indices_base_offset = 64 * copy_size_vertices;

	for (uint64_t i = 0; i < 64; ++i) {
		if (FRAME.model_update_flags & (1LL << i)) {

			vertex_copy_regions[num_pending_models].srcOffset = i * copy_size_vertices;
			vertex_copy_regions[num_pending_models].dstOffset = i * copy_size_vertices;
			vertex_copy_regions[num_pending_models].size = copy_size_vertices;

			index_copy_regions[num_pending_models].srcOffset = indices_base_offset + i * copy_size_indices;
			index_copy_regions[num_pending_models].dstOffset = i * copy_size_indices;
			index_copy_regions[num_pending_models].size = copy_size_indices;

			++num_pending_models;
		}
	}

	FRAME.model_update_flags = 0;

	vkCmdCopyBuffer(transfer_command_buffer, global_staging_buffer, FRAME.model_buffer.handle, num_pending_models, vertex_copy_regions);
	vkCmdCopyBuffer(index_transfer_command_buffer, global_staging_buffer, FRAME.index_buffer.handle, num_pending_models, index_copy_regions);

	vkEndCommandBuffer(index_transfer_command_buffer);
	vkEndCommandBuffer(transfer_command_buffer);

	VkSubmitInfo submit_info = { 0 };
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.commandBufferCount = 2;
	submit_info.pCommandBuffers = transfer_command_buffers;

	vkQueueSubmit(transfer_queue, 1, &submit_info, FRAME.fence_buffers_up_to_date);

	vkQueueWaitIdle(transfer_queue);

	vkFreeCommandBuffers(device, transfer_command_pool, 2, transfer_command_buffers);
}

// Dispatches a work load to the compute_matrices shader, computing a transformation matrix for each render object.
void compute_matrices(uint32_t num_inputs, float delta_time, projection_bounds_t projection_bounds, render_position_t camera_position, render_position_t *positions) {

	byte_t *compute_matrices_data;
	vkMapMemory(device, global_uniform_memory, 0, 256, 0, (void **)&compute_matrices_data);

	memcpy(compute_matrices_data, &num_inputs, sizeof num_inputs);
	memcpy(compute_matrices_data + 4, &delta_time, sizeof delta_time);
	memcpy(compute_matrices_data + 8, &projection_bounds, sizeof projection_bounds);
	memcpy(compute_matrices_data + 44, &camera_position, sizeof camera_position);
	memcpy(compute_matrices_data + 64, &positions, num_inputs * sizeof(render_position_t));

	vkUnmapMemory(device, global_uniform_memory);

	// Move this to global scope
	descriptor_pool_t descriptor_pool;
	create_descriptor_pool(device, 1, compute_matrices_layout, &descriptor_pool.handle);
	create_descriptor_set_layout(device, compute_matrices_layout, &descriptor_pool.layout);

	VkDescriptorSet descriptor_set;
	allocate_descriptor_sets(device, descriptor_pool, 1, &descriptor_set);

	VkDescriptorBufferInfo uniform_buffer_info = { 0 };
	uniform_buffer_info.buffer = global_uniform_buffer;
	uniform_buffer_info.offset = 0;
	uniform_buffer_info.range = 256;

	VkDescriptorBufferInfo matrix_buffer_info = { 0 };
	matrix_buffer_info.buffer = matrix_buffer.handle;
	matrix_buffer_info.offset = 0;
	matrix_buffer_info.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet descriptor_writes[2] = { { 0 } };

	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &uniform_buffer_info;
	descriptor_writes[0].pImageInfo = NULL;
	descriptor_writes[0].pTexelBufferView = NULL;

	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pBufferInfo = &matrix_buffer_info;
	descriptor_writes[1].pImageInfo = NULL;
	descriptor_writes[1].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device, 2, descriptor_writes, 0, NULL);

	VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, compute_command_pool, 1, &compute_command_buffer);

	VkCommandBufferBeginInfo begin_info = { 0 };
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(compute_command_buffer, &begin_info);

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_matrices.handle);
	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_matrices.layout, 0, 1, &descriptor_set, 0, NULL);

	vkCmdDispatch(compute_command_buffer, num_inputs, 1, 1);

	vkEndCommandBuffer(compute_command_buffer);
	submit_command_buffers_async(compute_queue, 1, &compute_command_buffer);
	vkFreeCommandBuffers(device, compute_command_pool, 1, &compute_command_buffer);

	destroy_descriptor_pool(device, descriptor_pool);
}

void compute_room_texture(extent_t room_extent, uint32_t *tile_data) {

	image_dimensions_t room_texture_dimensions = {
		room_extent.width * 16,
		room_extent.length * 16
	};

	queue_family_set_t queue_family_set_1 = {
		.num_queue_families = 2,
		.queue_families = (uint32_t[2]){
			*physical_device.queue_family_indices.transfer_family_ptr,
			*physical_device.queue_family_indices.compute_family_ptr,
		}
	};

	room_texture_storage = create_image(physical_device.handle, device, 
			room_texture_dimensions, 
			VK_FORMAT_R8G8B8A8_UINT,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			queue_family_set_1);

	transition_image_layout(graphics_queue, render_command_pool, &room_texture_storage, IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_GENERAL);



	VkDeviceSize tile_data_size = 16 * room_extent.width * room_extent.length;

	void *uniform_data;
	vkMapMemory(device, global_uniform_memory, 128, tile_data_size, 0, &uniform_data);

	memcpy(uniform_data, tile_data, tile_data_size);

	vkUnmapMemory(device, global_uniform_memory);

	descriptor_pool_t descriptor_pool;
	create_descriptor_pool(device, 1, compute_room_texture_layout, &descriptor_pool.handle);
	create_descriptor_set_layout(device, compute_room_texture_layout, &descriptor_pool.layout);

	VkDescriptorSet descriptor_set;
	allocate_descriptor_sets(device, descriptor_pool, 1, &descriptor_set);

	VkDescriptorBufferInfo uniform_buffer_info = make_descriptor_buffer_info(global_uniform_buffer, 128, tile_data_size);

	VkDescriptorImageInfo tilemap_texture_info = make_descriptor_image_info(no_sampler, tilemap_texture.view, tilemap_texture.layout);

	VkDescriptorImageInfo room_texture_storage_info = make_descriptor_image_info(sampler_default, room_texture_storage.view, room_texture_storage.layout);

	VkWriteDescriptorSet write_descriptor_sets[2] = { { 0 } };
	
	write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[0].pNext = NULL;
	write_descriptor_sets[0].dstSet = descriptor_set;
	write_descriptor_sets[0].dstBinding = 0;
	write_descriptor_sets[0].dstArrayElement = 0;
	write_descriptor_sets[0].descriptorCount = 1;
	write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[0].pImageInfo = NULL;
	write_descriptor_sets[0].pBufferInfo = &uniform_buffer_info;
	write_descriptor_sets[0].pTexelBufferView = NULL;

	VkDescriptorImageInfo storage_image_infos[2] = { tilemap_texture_info, room_texture_storage_info };

	write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[1].pNext = NULL;
	write_descriptor_sets[1].dstSet = descriptor_set;
	write_descriptor_sets[1].dstBinding = 1;
	write_descriptor_sets[1].dstArrayElement = 0;
	write_descriptor_sets[1].descriptorCount = 2;
	write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write_descriptor_sets[1].pImageInfo = storage_image_infos;
	write_descriptor_sets[1].pBufferInfo = NULL;
	write_descriptor_sets[1].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device, 2, write_descriptor_sets, 0, NULL);

	VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, compute_command_pool, 1, &compute_command_buffer);
	begin_command_buffer(compute_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_room_texture.handle);
	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_room_texture.layout, 0, 1, &descriptor_set, 0, NULL);

	vkCmdDispatch(compute_command_buffer, room_extent.width, room_extent.length, 1);

	vkEndCommandBuffer(compute_command_buffer);
	submit_command_buffers_async(compute_queue, 1, &compute_command_buffer);
	vkFreeCommandBuffers(device, compute_command_pool, 1, &compute_command_buffer);

	destroy_descriptor_pool(device, descriptor_pool);



	room_texture = create_image(physical_device.handle, device, 
			room_texture_dimensions,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			queue_family_set_null);

	transition_image_layout(graphics_queue, render_command_pool, &room_texture, IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_TRANSFER_DST); 

	VkCommandBuffer transfer_command_buffer_2 = VK_NULL_HANDLE;
	allocate_command_buffers(device, transfer_command_pool, 1, &transfer_command_buffer_2);
	begin_command_buffer(transfer_command_buffer_2, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageCopy copy_region_2 = { 0 };
	copy_region_2.srcSubresource = image_subresource_layers_default;
	copy_region_2.srcOffset = (VkOffset3D){ 0, 0, 0 };
	copy_region_2.dstSubresource = image_subresource_layers_default;
	copy_region_2.dstOffset = (VkOffset3D){ 0, 0, 0 };
	copy_region_2.extent = (VkExtent3D){ 
		.width = room_extent.width * 16, 
		.height = room_extent.length * 16, 
		.depth = 1 
	};

	vkCmdCopyImage(transfer_command_buffer_2, 
			room_texture_storage.handle, room_texture_storage.layout,
			room_texture.handle, room_texture.layout, 
			1, &copy_region_2);

	vkEndCommandBuffer(transfer_command_buffer_2);
	submit_command_buffers_async(transfer_queue, 1, &transfer_command_buffer_2);
	vkFreeCommandBuffers(device, transfer_command_pool, 1, &transfer_command_buffer_2);

	transition_image_layout(graphics_queue, render_command_pool, &room_texture, IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_SHADER_READ_ONLY); 
}

void create_room_texture(room_t room) {

	// Create the properly aligned tile data array, aligned to 16 bytes.

	const uint64_t num_tiles = room.extent.width * room.extent.length;
	static const uint64_t tile_datum_size = 16;	// Bytes per tile datum--the buffer is aligned to 16 bytes.

	uint32_t *tile_data = calloc(num_tiles, tile_datum_size);
	if (tile_data == NULL) {
		log_message(ERROR, "Allocation of aligned tile data array failed.");
		return;
	}

	for (uint64_t i = 0; i < num_tiles; ++i) {
		// The indexing is in increments of sizeof(uint32_t), not of 1 byte;
		// 	therefore, multiply i by the alignment size (16 bytes) then divided i by sizeof(uint32_t).
		uint64_t index = i * (tile_datum_size / sizeof(uint32_t));
		tile_data[index] = room.tiles[i].tilemap_slot;
	}

	compute_room_texture(room.extent, tile_data);

	free(tile_data);
}

void set_clear_color(color3F_t color) {
	clear_color.color.float32[0] = color.red;
	clear_color.color.float32[1] = color.green;
	clear_color.color.float32[2] = color.blue;
	clear_color.color.float32[3] = color.alpha;
}

// Send the drawing commands to the GPU to draw the frame.
void draw_frame(double delta_time, projection_bounds_t projection_bounds) {

	uint32_t image_index = 0;

	vkWaitForFences(device, 1, &FRAME.fence_frame_ready, VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(device, swapchain.handle, UINT64_MAX, FRAME.semaphore_image_available, VK_NULL_HANDLE, &image_index);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		// TODO - error handling
	}

	// If the buffers of this frame are not up-to-date, update them.
	if (vkGetFenceStatus(device, FRAME.fence_buffers_up_to_date) == VK_NOT_READY) {
		transfer_model_data();
	}


	// Compute matrices
	// Signal a semaphore when the entire batch in the compute queue is done being executed.

	render_position_t camera_position = { 0 };

	render_position_t pos0 = { { 0 }, { 0 } };

	render_position_t positions[1] = { pos0 };

	compute_matrices(1, (float)delta_time, projection_bounds, camera_position, positions);



	// Wait until the buffers are fully up-to-date.
	vkQueueWaitIdle(compute_queue);
	
	vkWaitForFences(device, 1, &FRAME.fence_buffers_up_to_date, VK_TRUE, UINT64_MAX);

	vkResetFences(device, 1, &FRAME.fence_frame_ready);

	vkResetCommandBuffer(FRAME.command_buffer, 0);

	VkWriteDescriptorSet descriptor_writes[2] = { { 0 } };

	VkDescriptorBufferInfo matrix_buffer_info = { 0 };
	matrix_buffer_info.buffer = matrix_buffer.handle;
	matrix_buffer_info.offset = 0;
	matrix_buffer_info.range = VK_WHOLE_SIZE;

	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = FRAME.descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &matrix_buffer_info;
	descriptor_writes[0].pImageInfo = NULL;
	descriptor_writes[0].pTexelBufferView = NULL;

	VkDescriptorImageInfo room_texture_info = { 0 };
	room_texture_info.sampler = sampler_default;
	room_texture_info.imageView = room_texture.view;
	room_texture_info.imageLayout = room_texture.layout;

	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = FRAME.descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pBufferInfo = NULL;
	descriptor_writes[1].pImageInfo = &room_texture_info;
	descriptor_writes[1].pTexelBufferView = NULL;
	
	vkUpdateDescriptorSets(device, 2, descriptor_writes, 0, NULL);



	VkCommandBufferBeginInfo begin_info = { 0 };
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(FRAME.command_buffer, &begin_info);

	VkRenderPassBeginInfo render_pass_info = { 0 };
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.pNext = NULL;
	render_pass_info.renderPass = graphics_pipeline.render_pass;
	render_pass_info.framebuffer = swapchain.framebuffers[image_index];
	render_pass_info.renderArea.offset.x = 0;
	render_pass_info.renderArea.offset.y = 0;
	render_pass_info.renderArea.extent = swapchain.extent;
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;

	vkCmdBeginRenderPass(FRAME.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(FRAME.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.handle);

	vkCmdBindDescriptorSets(FRAME.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.layout, 0, 1, &FRAME.descriptor_set, 0, NULL);

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(FRAME.command_buffer, 0, 1, &FRAME.model_buffer.handle, offsets);

	vkCmdBindIndexBuffer(FRAME.command_buffer, FRAME.index_buffer.handle, 0, VK_INDEX_TYPE_UINT16);

	const uint32_t model_slot = 0;
	vkCmdPushConstants(FRAME.command_buffer, graphics_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (sizeof model_slot), &model_slot);

	vkCmdDrawIndexed(FRAME.command_buffer, 6, 1, 0, 0, 0);

	vkCmdEndRenderPass(FRAME.command_buffer);

	vkEndCommandBuffer(FRAME.command_buffer);



	VkPipelineStageFlags wait_stages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submit_info = { 0 };
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &FRAME.command_buffer;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &FRAME.semaphore_image_available;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &FRAME.semaphore_render_finished;

	if (vkQueueSubmit(graphics_queue, 1, &submit_info, FRAME.fence_frame_ready) != VK_SUCCESS) {
		// TODO - error handling
	}



	VkPresentInfoKHR present_info = { 0 };
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &FRAME.semaphore_render_finished;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain.handle;
	present_info.pImageIndices = &image_index;
	present_info.pResults = NULL;

	vkQueuePresentKHR(present_queue, &present_info);

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}
