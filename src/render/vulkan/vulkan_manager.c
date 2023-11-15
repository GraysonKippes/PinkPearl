#include "vulkan_manager.h"

#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "debug.h"
#include "log/logging.h"
#include "glfw/glfw_manager.h"
#include "render/render_config.h"
#include "render/stb/image_data.h"
#include "util/bit.h"
#include "util/byte.h"

#include "buffer.h"
#include "command_buffer.h"
#include "compute_pipeline.h"
#include "descriptor.h"
#include "frame.h"
#include "graphics_pipeline.h"
#include "image.h"
#include "logical_device.h"
#include "memory.h"
#include "physical_device.h"
#include "queue.h"
#include "shader.h"
#include "swapchain.h"
#include "vertex_input.h"
#include "vulkan_instance.h"

/* -- Vulkan Objects -- */

static vulkan_instance_t vulkan_instance = { 0 };

static VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

static VkSurfaceKHR surface = VK_NULL_HANDLE;

static physical_device_t physical_device = { 0 };

static memory_type_set_t memory_type_set = { 0 };

static VkDevice logical_device = VK_NULL_HANDLE;

static swapchain_t swapchain = { 0 };

static graphics_pipeline_t graphics_pipeline = { 0 };

static VkSampler sampler_default = VK_NULL_HANDLE;

/* -- Memory -- */

static VkDeviceMemory graphics_memory = VK_NULL_HANDLE;

/* -- Compute -- */

#define NUM_COMPUTE_PIPELINES 3

static compute_pipeline_t compute_pipeline_matrices;
static compute_pipeline_t compute_pipeline_room_texture;
static compute_pipeline_t compute_pipeline_lighting;

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

static VkClearValue clear_color;

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

// Objects for room texture compute shader.
static buffer_t room_texture_uniform_buffer;
static image_t tilemap_texture;
static image_t room_texture_storage;
static image_t room_texture;

void compute_room_texture(void);



/* -- Function Definitions -- */

void set_clear_color(color3F_t color) {
	clear_color.color.float32[0] = color.red;
	clear_color.color.float32[1] = color.green;
	clear_color.color.float32[2] = color.blue;
	clear_color.color.float32[3] = color.alpha;
}

static void create_window_surface(void) {

	log_message(VERBOSE, "Creating window surface...");

	VkResult result = glfwCreateWindowSurface(vulkan_instance.m_handle, get_application_window(), NULL, &surface);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Window surface creation failed. (Error code: %i)", result);
		exit(1);
	}
}

static void allocate_device_memories(void) {

	log_message(VERBOSE, "Allocating device memories...");

	VkDeviceSize graphics_memory_size = 0;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {

		VkMemoryRequirements model_buffer_memory_requirements = get_buffer_memory_requirements(frames[i].m_model_buffer);
		VkMemoryRequirements index_buffer_memory_requirements = get_buffer_memory_requirements(frames[i].m_index_buffer);

		graphics_memory_size = model_buffer_memory_requirements.size + index_buffer_memory_requirements.size;
	}

	allocate_device_memory(logical_device, graphics_memory_size, memory_type_set.m_graphics_resources, &graphics_memory);

	VkDeviceSize graphics_memory_offset = 0;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {

		VkMemoryRequirements model_buffer_memory_requirements = get_buffer_memory_requirements(frames[i].m_model_buffer);
		bind_buffer_to_memory(frames[i].m_model_buffer, graphics_memory, graphics_memory_offset);
		graphics_memory_offset += model_buffer_memory_requirements.size;

		VkMemoryRequirements index_buffer_memory_requirements = get_buffer_memory_requirements(frames[i].m_index_buffer);
		bind_buffer_to_memory(frames[i].m_index_buffer, graphics_memory, graphics_memory_offset);
		graphics_memory_offset += index_buffer_memory_requirements.size;
	}
}

static void init_compute(void) {
	
	const uint32_t num_compute_shaders = NUM_COMPUTE_PIPELINES;

	compute_shader_t compute_shader_matrices = create_compute_shader(logical_device, compute_matrices_layout, "compute_matrices.spv");
	compute_shader_t compute_shader_room_texture = create_compute_shader(logical_device, compute_room_texture_layout, "room_texture.spv");
	compute_shader_t compute_shader_lighting = create_compute_shader(logical_device, compute_textures_layout, "compute_textures.spv");

	VkPipeline compute_pipelines[NUM_COMPUTE_PIPELINES];
	VkPipelineLayout compute_pipeline_layouts[NUM_COMPUTE_PIPELINES];

	create_compute_pipelines(logical_device, compute_pipelines, compute_pipeline_layouts, num_compute_shaders, 
			compute_shader_matrices, compute_shader_room_texture, compute_shader_lighting);

	compute_pipeline_matrices.m_handle = compute_pipelines[0];
	compute_pipeline_matrices.m_layout = compute_pipeline_layouts[0];
	compute_pipeline_matrices.m_descriptor_set_layout = compute_shader_matrices.m_descriptor_set_layout;

	compute_pipeline_room_texture.m_handle = compute_pipelines[1];
	compute_pipeline_room_texture.m_layout = compute_pipeline_layouts[1];
	compute_pipeline_room_texture.m_descriptor_set_layout = compute_shader_room_texture.m_descriptor_set_layout;

	compute_pipeline_lighting.m_handle = compute_pipelines[2];
	compute_pipeline_lighting.m_layout = compute_pipeline_layouts[2];
	compute_pipeline_lighting.m_descriptor_set_layout = compute_shader_lighting.m_descriptor_set_layout;

	destroy_compute_shader(logical_device, compute_shader_matrices);
	destroy_compute_shader(logical_device, compute_shader_room_texture);
	destroy_compute_shader(logical_device, compute_shader_lighting);
}

static buffer_t image_staging_buffer;

static void create_buffers(void) {

	VkDeviceSize staging_buffer_size = 4096;

	image_staging_buffer = create_buffer(physical_device.m_handle, logical_device, staging_buffer_size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			queue_family_set_null);

	static const VkDeviceSize num_elements_per_rect = 4 * 8;

	static const VkDeviceSize max_num_models = 64;

	// General vertex buffer size
	const VkDeviceSize model_buffer_size = max_num_models * num_elements_per_rect * sizeof(float);

	model_staging_buffer = create_buffer(physical_device.m_handle, logical_device, model_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			queue_family_set_null);

	const VkDeviceSize index_buffer_size = max_num_models * 6 * sizeof(index_t);

	index_staging_buffer = create_buffer(physical_device.m_handle, logical_device, index_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			queue_family_set_null);

	compute_matrices_buffer = create_buffer(physical_device.m_handle, logical_device, 68, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			queue_family_set_null);

	VkDeviceSize render_positions_buffer_size = 64 * sizeof(render_position_t);

	render_positions_buffer = create_buffer(physical_device.m_handle, logical_device, render_positions_buffer_size, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			queue_family_set_null);

	VkDeviceSize matrix_buffer_size = 64 * 16 * sizeof(float);

	matrix_buffer = create_buffer(physical_device.m_handle, logical_device, matrix_buffer_size, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			queue_family_set_null);
}

static void create_images(void) {

	// Apparently the staging buffer can't be created if the image data is loaded first...
	image_data_t image_data = load_image_data("../resources/assets/textures/tilemap/dungeon2.png");
	
	map_data_to_whole_buffer(logical_device, image_staging_buffer, image_data.m_data);

	// TEMP
	room_texture_uniform_buffer = create_buffer(physical_device.m_handle, logical_device, 128, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			queue_family_set_null);

	create_sampler(physical_device, logical_device, &sampler_default);

	image_dimensions_t tilemap_texture_dimensions = { (uint32_t)image_data.m_width, (uint32_t)image_data.m_height };

	queue_family_set_t queue_family_set_0 = {
		.m_num_queue_families = 2,
		.m_queue_families = (uint32_t[2]){
			*physical_device.m_queue_family_indices.m_transfer_family_ptr,
			*physical_device.m_queue_family_indices.m_compute_family_ptr,
		}
	};

	tilemap_texture = create_image(physical_device.m_handle, logical_device, 
			tilemap_texture_dimensions, 
			VK_FORMAT_R8G8B8A8_UINT, 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			queue_family_set_0);

	transition_image_layout(graphics_queue, render_command_pool, &tilemap_texture, IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_TRANSFER_DST);

	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(logical_device, transfer_command_pool, 1, &transfer_command_buffer);
	begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkBufferImageCopy2 copy_region = make_buffer_image_copy((VkOffset2D){ 0 }, (VkExtent2D){ 32, 32 });

	VkCopyBufferToImageInfo2 copy_info = { 0 };
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
	copy_info.pNext = NULL;
	copy_info.srcBuffer = image_staging_buffer.m_handle;
	copy_info.dstImage = tilemap_texture.m_handle;
	copy_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	copy_info.regionCount = 1;
	copy_info.pRegions = &copy_region;

	vkCmdCopyBufferToImage2(transfer_command_buffer, &copy_info);

	vkEndCommandBuffer(transfer_command_buffer);
	submit_command_buffers_async(transfer_queue, 1, &transfer_command_buffer);
	vkFreeCommandBuffers(logical_device, transfer_command_pool, 1, &transfer_command_buffer);

	destroy_buffer(&image_staging_buffer);
	free_image_data(image_data);

	// Transition tilemap texture layout so that it can be read by the compute shader.
	transition_image_layout(graphics_queue, render_command_pool, &tilemap_texture, IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_GENERAL);

	image_dimensions_t room_texture_dimensions = {
		8 * 16,
		5 * 16
	};

	queue_family_set_t queue_family_set_1 = {
		.m_num_queue_families = 2,
		.m_queue_families = (uint32_t[2]){
			*physical_device.m_queue_family_indices.m_transfer_family_ptr,
			*physical_device.m_queue_family_indices.m_compute_family_ptr,
		}
	};

	room_texture_storage = create_image(physical_device.m_handle, logical_device, 
			room_texture_dimensions, 
			VK_FORMAT_R8G8B8A8_UINT,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			queue_family_set_1);

	transition_image_layout(graphics_queue, render_command_pool, &room_texture_storage, IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_GENERAL);



	compute_room_texture();



	room_texture = create_image(physical_device.m_handle, logical_device, 
			room_texture_dimensions,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			queue_family_set_null);

	transition_image_layout(graphics_queue, render_command_pool, &room_texture, IMAGE_LAYOUT_TRANSITION_UNDEFINED_TO_TRANSFER_DST); 

	VkCommandBuffer transfer_command_buffer_2 = VK_NULL_HANDLE;
	allocate_command_buffers(logical_device, transfer_command_pool, 1, &transfer_command_buffer_2);
	begin_command_buffer(transfer_command_buffer_2, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkImageCopy copy_region_2 = { 0 };
	copy_region_2.srcSubresource = image_subresource_layers_default;
	copy_region_2.srcOffset = (VkOffset3D){ 0, 0, 0 };
	copy_region_2.dstSubresource = image_subresource_layers_default;
	copy_region_2.dstOffset = (VkOffset3D){ 0, 0, 0 };
	copy_region_2.extent = (VkExtent3D){ 8 * 16, 5 * 16, 1 };

	vkCmdCopyImage(transfer_command_buffer_2, 
			room_texture_storage.m_handle, room_texture_storage.m_layout,
			room_texture.m_handle, room_texture.m_layout, 
			1, &copy_region_2);

	vkEndCommandBuffer(transfer_command_buffer);
	submit_command_buffers_async(transfer_queue, 1, &transfer_command_buffer);
	vkFreeCommandBuffers(logical_device, transfer_command_pool, 1, &transfer_command_buffer);

	transition_image_layout(graphics_queue, render_command_pool, &room_texture, IMAGE_LAYOUT_TRANSITION_TRANSFER_DST_TO_SHADER_READ_ONLY); 
}

void create_vulkan_objects(void) {

	log_message(INFO, "Initializing Vulkan...");

	vulkan_instance = create_vulkan_instance();

	if (debug_enabled) {
		setup_debug_messenger(vulkan_instance.m_handle, &debug_messenger);
	}

	create_window_surface();

	physical_device = select_physical_device(vulkan_instance.m_handle, surface);

	memory_type_set = select_memory_types(physical_device.m_handle);

	create_logical_device(vulkan_instance, physical_device, &logical_device);

	log_message(VERBOSE, "Retrieving device queues...");

	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_graphics_family_ptr, 0, &graphics_queue);
	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_present_family_ptr, 0, &present_queue);
	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_transfer_family_ptr, 0, &transfer_queue);
	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_compute_family_ptr, 0, &compute_queue);

	create_command_pool(logical_device, default_command_pool_flags, *physical_device.m_queue_family_indices.m_graphics_family_ptr, &render_command_pool);
	create_command_pool(logical_device, transfer_command_pool_flags, *physical_device.m_queue_family_indices.m_transfer_family_ptr, &transfer_command_pool);
	create_command_pool(logical_device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, *physical_device.m_queue_family_indices.m_compute_family_ptr, &compute_command_pool);

	swapchain = create_swapchain(get_application_window(), surface, physical_device, logical_device, VK_NULL_HANDLE);

	clear_color = (VkClearValue){ { { 0.0F, 0.0F, 0.0F, 1.0F } } };

	VkShaderModule vertex_shader;
	create_shader_module(logical_device, "vertex_models.spv", &vertex_shader);

	VkShaderModule fragment_shader;
	create_shader_module(logical_device, "fragment_shader_2.spv", &fragment_shader);

	VkDescriptorSetLayout graphics_pipeline_layout = VK_NULL_HANDLE;
	create_descriptor_set_layout(logical_device, graphics_descriptor_layout, &graphics_pipeline_layout);

	graphics_pipeline = create_graphics_pipeline(logical_device, swapchain, graphics_pipeline_layout, vertex_shader, fragment_shader);

	vkDestroyShaderModule(logical_device, vertex_shader, NULL);
	vkDestroyShaderModule(logical_device, fragment_shader, NULL);

	create_framebuffers(logical_device, graphics_pipeline.m_render_pass, &swapchain);

	init_compute();

	create_descriptor_pool(logical_device, MAX_FRAMES_IN_FLIGHT, graphics_descriptor_layout, &graphics_descriptor_pool.m_handle);
	create_descriptor_set_layout(logical_device, graphics_descriptor_layout, &graphics_descriptor_pool.m_layout);

	log_message(VERBOSE, "Creating frame objects...");

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		frames[i] = create_frame(physical_device, logical_device, render_command_pool, graphics_descriptor_pool);
	}

	allocate_device_memories();

	create_buffers();
	create_images();
}

void destroy_vulkan_objects(void) {

	vkDeviceWaitIdle(logical_device);

	destroy_buffer(&compute_matrices_buffer);
	destroy_buffer(&render_positions_buffer);
	destroy_buffer(&matrix_buffer);
	destroy_buffer(&index_staging_buffer);
	destroy_buffer(&model_staging_buffer);
	destroy_buffer(&room_texture_uniform_buffer);
	
	destroy_image(tilemap_texture);
	destroy_image(room_texture_storage);

	vkDestroySampler(logical_device, sampler_default, NULL);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		destroy_frame(logical_device, render_command_pool, graphics_descriptor_pool, frames[i]);
	}

	destroy_compute_pipeline(logical_device, compute_pipeline_matrices);
	destroy_compute_pipeline(logical_device, compute_pipeline_room_texture);
	destroy_compute_pipeline(logical_device, compute_pipeline_lighting);

	destroy_descriptor_pool(logical_device, graphics_descriptor_pool);
	destroy_graphics_pipeline(logical_device, graphics_pipeline);

	destroy_swapchain(logical_device, swapchain);

	vkDestroyCommandPool(logical_device, render_command_pool, NULL);
	vkDestroyCommandPool(logical_device, transfer_command_pool, NULL);
	vkDestroyCommandPool(logical_device, compute_command_pool, NULL);

	vkDestroyDevice(logical_device, NULL);

	vkDestroySurfaceKHR(vulkan_instance.m_handle, surface, NULL);

	destroy_debug_messenger(vulkan_instance.m_handle, debug_messenger);

	destroy_vulkan_instance(vulkan_instance);
}

// Copy the model data to the appropriate staging buffers, and signal the render engine to transfer that data to the buffers on the GPU.
void stage_model_data(render_handle_t handle, model_t model) {

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkResetFences(logical_device, 1, &frames[i].m_fence_buffers_up_to_date);
		frames[i].m_model_update_flags |= (1LL << (uint64_t)handle);
	}

	VkDeviceSize model_size = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	VkDeviceSize model_offset = handle * model_size;

	void *model_staging_data;
	vkMapMemory(logical_device, model_staging_buffer.m_memory, model_offset, model_size, 0, &model_staging_data);
	memcpy(model_staging_data, model.m_vertices, model_size);
	vkUnmapMemory(logical_device, model_staging_buffer.m_memory);

	VkDeviceSize indices_size = num_indices_per_rect * sizeof(index_t);
	VkDeviceSize indices_offset = handle * indices_size;

	index_t rect_indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	for (size_t i = 0; i < num_indices_per_rect; ++i) {
		rect_indices[i] += handle * 4;
	}

	void *index_staging_data;
	vkMapMemory(logical_device, index_staging_buffer.m_memory, indices_offset, indices_size, 0, &index_staging_data);
	memcpy(index_staging_data, rect_indices, indices_size);
	vkUnmapMemory(logical_device, index_staging_buffer.m_memory);
}

// Dispatches a work load to the compute_matrices shader, computing a transformation matrix for each render object.
void compute_matrices(uint32_t num_inputs, float delta_time, render_position_t camera_position, projection_bounds_t projection_bounds, render_position_t *positions) {

	byte_t *compute_matrices_data;
	vkMapMemory(logical_device, compute_matrices_buffer.m_memory, 0, 68, 0, (void **)&compute_matrices_data);

	memcpy(compute_matrices_data, &num_inputs, sizeof num_inputs);
	memcpy(compute_matrices_data + 4, &delta_time, sizeof delta_time);
	memcpy(compute_matrices_data + 16, &camera_position, sizeof camera_position);
	memcpy(compute_matrices_data + 44, &projection_bounds, sizeof projection_bounds);

	vkUnmapMemory(logical_device, compute_matrices_buffer.m_memory);

	// Move this to global scope
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

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_matrices.m_handle);
	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_matrices.m_layout, 0, 1, &descriptor_set, 0, NULL);

	vkCmdDispatch(compute_command_buffer, num_inputs, 1, 1);

	vkEndCommandBuffer(compute_command_buffer);
	submit_command_buffers_async(compute_queue, 1, &compute_command_buffer);
	vkFreeCommandBuffers(logical_device, compute_command_pool, 1, &compute_command_buffer);

	destroy_descriptor_pool(logical_device, descriptor_pool);
}

#include <stdalign.h>

void compute_room_texture(void) {

	uint32_t room_width = 8;
	uint32_t room_height = 5;

	alignas(16) uint32_t room_tiles[40] = {
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1
	};

	typedef struct room_tile_index_t {
		uint32_t m_index;
		byte_t m_reserved[12];
	} room_tile_index_t;

	alignas(16) room_tile_index_t room_tiles_3[4];
	room_tiles_3[0] = (room_tile_index_t){1};
	room_tiles_3[1] = (room_tile_index_t){2};
	room_tiles_3[2] = (room_tile_index_t){3};
	room_tiles_3[3] = (room_tile_index_t){2};

	map_data_to_buffer(logical_device, room_texture_uniform_buffer, 0, 16 * 4, room_tiles_3);

	descriptor_pool_t descriptor_pool;
	create_descriptor_pool(logical_device, 1, compute_room_texture_layout, &descriptor_pool.m_handle);
	create_descriptor_set_layout(logical_device, compute_room_texture_layout, &descriptor_pool.m_layout);

	VkDescriptorSet descriptor_set;
	allocate_descriptor_sets(logical_device, descriptor_pool, 1, &descriptor_set);

	VkDescriptorBufferInfo room_texture_uniform_buffer_info = make_descriptor_buffer_info(room_texture_uniform_buffer.m_handle, 0, VK_WHOLE_SIZE);

	VkDescriptorImageInfo tilemap_texture_info = make_descriptor_image_info(no_sampler, tilemap_texture.m_view, tilemap_texture.m_layout);

	VkDescriptorImageInfo room_texture_storage_info = make_descriptor_image_info(sampler_default, room_texture_storage.m_view, room_texture_storage.m_layout);

	VkWriteDescriptorSet write_descriptor_sets[2] = { { 0 } };
	
	write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[0].pNext = NULL;
	write_descriptor_sets[0].dstSet = descriptor_set;
	write_descriptor_sets[0].dstBinding = 0;
	write_descriptor_sets[0].dstArrayElement = 0;
	write_descriptor_sets[0].descriptorCount = 1;
	write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[0].pImageInfo = NULL;
	write_descriptor_sets[0].pBufferInfo = &room_texture_uniform_buffer_info;
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

	vkUpdateDescriptorSets(logical_device, 2, write_descriptor_sets, 0, NULL);

	VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(logical_device, compute_command_pool, 1, &compute_command_buffer);
	begin_command_buffer(compute_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_room_texture.m_handle);
	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_room_texture.m_layout, 0, 1, &descriptor_set, 0, NULL);

	vkCmdDispatch(compute_command_buffer, room_width, room_height, 1);

	vkEndCommandBuffer(compute_command_buffer);
	submit_command_buffers_async(compute_queue, 1, &compute_command_buffer);
	vkFreeCommandBuffers(logical_device, compute_command_pool, 1, &compute_command_buffer);

	destroy_descriptor_pool(logical_device, descriptor_pool);
}

// Send the drawing commands to the GPU to draw the frame.
void draw_frame(double delta_time, projection_bounds_t projection_bounds) {

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

		begin_command_buffer(transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		begin_command_buffer(index_transfer_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VkBufferCopy copy_regions[64] = { { 0 } };
		VkBufferCopy index_copy_regions[64] = { { 0 } };

		// The number of models needing to be updated.
		uint32_t num_pending_models = 0;

		const VkDeviceSize copy_size_vertices = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
		const VkDeviceSize copy_size_indices = num_indices_per_rect * sizeof(index_t);

		for (uint64_t i = 0; i < 64; ++i) {
			if (FRAME.m_model_update_flags & (1LL << i)) {

				copy_regions[num_pending_models].srcOffset = i * copy_size_vertices;
				copy_regions[num_pending_models].dstOffset = i * copy_size_vertices;
				copy_regions[num_pending_models].size = copy_size_vertices;

				index_copy_regions[num_pending_models].srcOffset = i * copy_size_indices;
				index_copy_regions[num_pending_models].dstOffset = i * copy_size_indices;
				index_copy_regions[num_pending_models].size = copy_size_indices;

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

	render_position_t pos0 = { { 0 }, { 0 } };

	render_position_t positions[1] = { pos0 };

	compute_matrices(1, (float)delta_time, camera_position, projection_bounds, positions);



	// Wait until the buffers are fully up-to-date.
	vkQueueWaitIdle(compute_queue);
	
	vkWaitForFences(logical_device, 1, &FRAME.m_fence_buffers_up_to_date, VK_TRUE, UINT64_MAX);

	vkResetFences(logical_device, 1, &FRAME.m_fence_frame_ready);

	vkResetCommandBuffer(FRAME.m_command_buffer, 0);

	VkWriteDescriptorSet descriptor_writes[2] = { { 0 } };

	VkDescriptorBufferInfo matrix_buffer_info = { 0 };
	matrix_buffer_info.buffer = matrix_buffer.m_handle;
	matrix_buffer_info.offset = 0;
	matrix_buffer_info.range = VK_WHOLE_SIZE;

	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = FRAME.m_descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &matrix_buffer_info;
	descriptor_writes[0].pImageInfo = NULL;
	descriptor_writes[0].pTexelBufferView = NULL;

	VkDescriptorImageInfo room_texture_info = { 0 };
	room_texture_info.sampler = sampler_default;
	room_texture_info.imageView = room_texture.m_view;
	room_texture_info.imageLayout = room_texture.m_layout;

	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = FRAME.m_descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pBufferInfo = NULL;
	descriptor_writes[1].pImageInfo = &room_texture_info;
	descriptor_writes[1].pTexelBufferView = NULL;
	
	vkUpdateDescriptorSets(logical_device, 2, descriptor_writes, 0, NULL);



	VkCommandBufferBeginInfo begin_info = { 0 };
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(FRAME.m_command_buffer, &begin_info);

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
	vkCmdPushConstants(FRAME.m_command_buffer, graphics_pipeline.m_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (sizeof model_slot), &model_slot);

	vkCmdDrawIndexed(FRAME.m_command_buffer, 6, 1, 0, 0, 0);

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
