#include "vulkan_manager.h"

#include <stdalign.h>
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

VkSurfaceKHR surface = VK_NULL_HANDLE;

physical_device_t physical_device = { 0 };

memory_type_set_t memory_type_set = { 0 };

VkDevice device = VK_NULL_HANDLE;

swapchain_t swapchain = { 0 };

graphics_pipeline_t graphics_pipeline = { 0 };

VkSampler sampler_default = VK_NULL_HANDLE;

/* -- Memory -- */

static VkDeviceMemory graphics_memory = VK_NULL_HANDLE;

/* -- Compute -- */

compute_pipeline_t compute_pipeline_matrices;
compute_pipeline_t compute_pipeline_room_texture;
compute_pipeline_t compute_pipeline_lighting;

/* -- Queues -- */

VkQueue graphics_queue;
VkQueue present_queue;
VkQueue transfer_queue;
VkQueue compute_queue;

/* -- Command Pools -- */

VkCommandPool render_command_pool;
VkCommandPool transfer_command_pool;
VkCommandPool compute_command_pool;



/* -- Render Objects -- */

static VkClearValue clear_color;

#define MAX_FRAMES_IN_FLIGHT	2

frame_t frames[MAX_FRAMES_IN_FLIGHT];

size_t current_frame = 0;

#define FRAME (frames[current_frame])



/* -- Global buffers -- */

// Used for uniform data into both compute shaders and graphics (vertex, fragment) shaders.
VkBuffer global_uniform_buffer = VK_NULL_HANDLE;
const VkDeviceSize global_uniform_buffer_size = 256 + 10240;
VkDeviceMemory global_uniform_memory = VK_NULL_HANDLE;



// TODO - move this to graphics pipeline struct
static descriptor_pool_t graphics_descriptor_pool = { 0 };




/* -- Function Definitions -- */

static void create_window_surface(void) {

	log_message(VERBOSE, "Creating window surface...");

	VkResult result = glfwCreateWindowSurface(vulkan_instance.handle, get_application_window(), NULL, &surface);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Window surface creation failed. (Error code: %i)", result);
		exit(1);
	}
}

static void create_global_uniform_buffer(void) {

	log_message(VERBOSE, "Creating global uniform buffer...");

	VkBufferCreateInfo buffer_create_info = { 0 };
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = NULL;
	buffer_create_info.flags = 0;
	buffer_create_info.size = global_uniform_buffer_size;
	buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = NULL;

	// TODO - error handling
	VkResult result = vkCreateBuffer(device, &buffer_create_info, NULL, &global_uniform_buffer);
	if (result != VK_SUCCESS) {
		logf_message(ERROR, "Global buffer creation failed. (Error code: %i)", result);
		return;
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, global_uniform_buffer, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = memory_type_set.uniform_data;

	// TODO - error handling
	vkAllocateMemory(device, &allocate_info, NULL, &global_uniform_memory);

	vkBindBufferMemory(device, global_uniform_buffer, global_uniform_memory, 0);
}

static void allocate_device_memories(void) {

	log_message(VERBOSE, "Allocating device memories...");

	VkDeviceSize graphics_memory_size = 0;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {

		VkMemoryRequirements model_buffer_memory_requirements = get_buffer_memory_requirements(frames[i].model_buffer);
		VkMemoryRequirements index_buffer_memory_requirements = get_buffer_memory_requirements(frames[i].index_buffer);

		graphics_memory_size = model_buffer_memory_requirements.size + index_buffer_memory_requirements.size;
	}

	allocate_device_memory(device, graphics_memory_size, memory_type_set.graphics_resources, &graphics_memory);

	VkDeviceSize graphics_memory_offset = 0;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {

		VkMemoryRequirements model_buffer_memory_requirements = get_buffer_memory_requirements(frames[i].model_buffer);
		bind_buffer_to_memory(frames[i].model_buffer, graphics_memory, graphics_memory_offset);
		graphics_memory_offset += model_buffer_memory_requirements.size;

		VkMemoryRequirements index_buffer_memory_requirements = get_buffer_memory_requirements(frames[i].index_buffer);
		bind_buffer_to_memory(frames[i].index_buffer, graphics_memory, graphics_memory_offset);
		graphics_memory_offset += index_buffer_memory_requirements.size;
	}
}

static void init_compute(void) {

	compute_pipeline_matrices = create_compute_pipeline(device, compute_matrices_layout, "compute_matrices.spv");

	compute_pipeline_room_texture = create_compute_pipeline(device, compute_room_texture_layout, "room_texture.spv");

	compute_pipeline_lighting = create_compute_pipeline(device, compute_textures_layout, "compute_textures.spv");

}

void create_vulkan_objects(void) {

	log_message(INFO, "Initializing Vulkan...");

	vulkan_instance = create_vulkan_instance();

	if (debug_enabled) {
		setup_debug_messenger(vulkan_instance.handle, &debug_messenger);
	}

	create_window_surface();

	physical_device = select_physical_device(vulkan_instance.handle, surface);
	memory_type_set = select_memory_types(physical_device.handle);

	create_device(vulkan_instance, physical_device, &device);

	create_global_uniform_buffer();

	log_message(VERBOSE, "Retrieving device queues...");

	vkGetDeviceQueue(device, *physical_device.queue_family_indices.graphics_family_ptr, 0, &graphics_queue);
	vkGetDeviceQueue(device, *physical_device.queue_family_indices.present_family_ptr, 0, &present_queue);
	vkGetDeviceQueue(device, *physical_device.queue_family_indices.transfer_family_ptr, 0, &transfer_queue);
	vkGetDeviceQueue(device, *physical_device.queue_family_indices.compute_family_ptr, 0, &compute_queue);

	create_command_pool(device, default_command_pool_flags, *physical_device.queue_family_indices.graphics_family_ptr, &render_command_pool);
	create_command_pool(device, transfer_command_pool_flags, *physical_device.queue_family_indices.transfer_family_ptr, &transfer_command_pool);
	create_command_pool(device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, *physical_device.queue_family_indices.compute_family_ptr, &compute_command_pool);

	swapchain = create_swapchain(get_application_window(), surface, physical_device, device, VK_NULL_HANDLE);

	clear_color = (VkClearValue){ { { 0.0F, 0.0F, 0.0F, 1.0F } } };

	VkShaderModule vertex_shader;
	create_shader_module(device, "vertex_models.spv", &vertex_shader);

	VkShaderModule fragment_shader;
	create_shader_module(device, "fragment_shader_2.spv", &fragment_shader);

	VkDescriptorSetLayout graphics_pipeline_layout = VK_NULL_HANDLE;
	create_descriptor_set_layout(device, graphics_descriptor_layout, &graphics_pipeline_layout);

	graphics_pipeline = create_graphics_pipeline(device, swapchain, graphics_pipeline_layout, vertex_shader, fragment_shader);

	vkDestroyShaderModule(device, vertex_shader, NULL);
	vkDestroyShaderModule(device, fragment_shader, NULL);

	create_framebuffers(device, graphics_pipeline.render_pass, &swapchain);

	init_compute();

	create_descriptor_pool(device, MAX_FRAMES_IN_FLIGHT, graphics_descriptor_layout, &graphics_descriptor_pool.handle);
	create_descriptor_set_layout(device, graphics_descriptor_layout, &graphics_descriptor_pool.layout);

	log_message(VERBOSE, "Creating frame objects...");

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		frames[i] = create_frame(physical_device, device, render_command_pool, graphics_descriptor_pool);
	}
}

void destroy_vulkan_objects(void) {

	vkDeviceWaitIdle(device);

	vkDestroySampler(device, sampler_default, NULL);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		destroy_frame(device, render_command_pool, graphics_descriptor_pool, frames[i]);
	}

	destroy_compute_pipeline(device, compute_pipeline_matrices);
	destroy_compute_pipeline(device, compute_pipeline_room_texture);
	destroy_compute_pipeline(device, compute_pipeline_lighting);

	destroy_descriptor_pool(device, graphics_descriptor_pool);
	destroy_graphics_pipeline(device, graphics_pipeline);

	destroy_swapchain(device, swapchain);

	vkDestroyCommandPool(device, render_command_pool, NULL);
	vkDestroyCommandPool(device, transfer_command_pool, NULL);
	vkDestroyCommandPool(device, compute_command_pool, NULL);

	vkDestroyBuffer(device, global_uniform_buffer, NULL);
	vkFreeMemory(device, global_uniform_memory, NULL);

	vkDestroyDevice(device, NULL);

	vkDestroySurfaceKHR(vulkan_instance.handle, surface, NULL);

	destroy_debug_messenger(vulkan_instance.handle, debug_messenger);

	destroy_vulkan_instance(vulkan_instance);
}
