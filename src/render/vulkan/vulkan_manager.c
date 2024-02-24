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

/* -- Compute -- */

// TODO - get this out of here.
compute_pipeline_t compute_pipeline_matrices;

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

frame_t frames[NUM_FRAMES_IN_FLIGHT];
size_t current_frame = 0;

#define FRAME (frames[current_frame])



/* -- Global buffers -- */

buffer_partition_t global_staging_buffer_partition;
buffer_partition_t global_uniform_buffer_partition;
buffer_partition_t global_storage_buffer_partition;



/* -- Function Definitions -- */

static void create_window_surface(void) {

	log_message(VERBOSE, "Creating window surface...");

	VkResult result = glfwCreateWindowSurface(vulkan_instance.handle, get_application_window(), NULL, &surface);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Window surface creation failed. (Error code: %i)", result);
		// TODO - do not use exit() here.
		exit(1);
	}
}

static void create_global_staging_buffer(void) {

	log_message(VERBOSE, "Creating global staging buffer...");

	const buffer_partition_create_info_t buffer_partition_create_info = {
		.physical_device = physical_device.handle,
		.device = device,
		.buffer_type = BUFFER_TYPE_STAGING,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = NULL,
		.num_partition_sizes = 2,
		.partition_sizes = (VkDeviceSize[2]){
			5120,	// Render object model data
			262144	// Loaded image data

		}
	};
	
	global_staging_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

static void create_global_uniform_buffer(void) {

	log_message(VERBOSE, "Creating global uniform buffer...");

	const buffer_partition_create_info_t buffer_partition_create_info = {
		.physical_device = physical_device.handle,
		.device = device,
		.buffer_type = BUFFER_TYPE_UNIFORM,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = NULL,
		.num_partition_sizes = 3,
		.partition_sizes = (VkDeviceSize[3]){
			2096,	// Compute matrices
			10240,	// Compute room texture
			8200	// Compute area mesh
		}
	};
	
	global_uniform_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

static void create_global_storage_buffer(void) {

	log_message(VERBOSE, "Creating global storage buffer...");

	const buffer_partition_create_info_t buffer_partition_create_info = {
		.physical_device = physical_device.handle,
		.device = device,
		.buffer_type = BUFFER_TYPE_STORAGE,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = NULL,
		.num_partition_sizes = 3,
		.partition_sizes = (VkDeviceSize[3]){
			4224,	// Compute matrices
			81920,	// Area mesh data--vertices
			12288	// Area mesh data--indices
		}
	};
	
	global_storage_buffer_partition = create_buffer_partition(buffer_partition_create_info);
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

	create_global_staging_buffer();
	create_global_uniform_buffer();
	create_global_storage_buffer();

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
	create_shader_module(device, VERTEX_SHADER_NAME, &vertex_shader);

	VkShaderModule fragment_shader;
	create_shader_module(device, FRAGMENT_SHADER_NAME, &fragment_shader);

	graphics_pipeline = create_graphics_pipeline(device, swapchain, graphics_descriptor_set_layout, vertex_shader, fragment_shader);

	vkDestroyShaderModule(device, vertex_shader, NULL);
	vkDestroyShaderModule(device, fragment_shader, NULL);

	create_framebuffers(device, graphics_pipeline.render_pass, &swapchain);

	log_message(VERBOSE, "Creating frame objects...");

	for (size_t i = 0; i < num_frames_in_flight; ++i) {
		frames[i] = create_frame(physical_device, device, 
				render_command_pool, 
				graphics_pipeline.descriptor_pool, graphics_pipeline.descriptor_set_layout);
	}

	create_sampler(physical_device, device, &sampler_default);
}

void destroy_vulkan_objects(void) {

	log_message(VERBOSE, "Destroying Vulkan objects...");

	vkDestroySampler(device, sampler_default, NULL);

	for (uint32_t i = 0; i < num_frames_in_flight; ++i) {
		destroy_frame(device, frames[i]);
	}

	destroy_graphics_pipeline(device, graphics_pipeline);
	destroy_swapchain(device, swapchain);

	vkDestroyCommandPool(device, render_command_pool, NULL);
	vkDestroyCommandPool(device, transfer_command_pool, NULL);
	vkDestroyCommandPool(device, compute_command_pool, NULL);

	destroy_buffer_partition(&global_staging_buffer_partition);
	destroy_buffer_partition(&global_uniform_buffer_partition);
	destroy_buffer_partition(&global_storage_buffer_partition);

	vkDestroyDevice(device, NULL);
	vkDestroySurfaceKHR(vulkan_instance.handle, surface, NULL);
	destroy_debug_messenger(vulkan_instance.handle, debug_messenger);
	destroy_vulkan_instance(vulkan_instance);
}
