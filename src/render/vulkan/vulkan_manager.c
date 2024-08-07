#include "vulkan_manager.h"

#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "debug.h"
#include "log/Logger.h"
#include "glfw/glfw_manager.h"
#include "render/render_config.h"
#include "render/stb/image_data.h"
#include "util/byte.h"

#include "CommandBuffer.h"
#include "descriptor.h"
#include "image.h"
#include "logical_device.h"
#include "queue.h"
#include "shader.h"
#include "vertex_input.h"
#include "vulkan_instance.h"

/* -- Vulkan Module Configuration -- */

const int vkConfMaxNumQuads = VK_CONF_MAX_NUM_QUADS;

/* -- Vulkan Objects -- */

static vulkan_instance_t vulkan_instance = { };
static VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
VkSurfaceKHR surface = VK_NULL_HANDLE;
physical_device_t physical_device = { };
memory_type_set_t memory_type_set = { };
VkDevice device = VK_NULL_HANDLE;
Swapchain swapchain = { };
GraphicsPipeline graphics_pipeline = { };
VkRenderPass renderPass = VK_NULL_HANDLE;
VkSampler imageSamplerDefault = VK_NULL_HANDLE;

/* -- Queues -- */

VkQueue queueGraphics;
VkQueue queuePresent;
VkQueue queueTransfer;
VkQueue queueCompute;

/* -- Command Pools -- */

CommandPool commandPoolGraphics;
CommandPool commandPoolTransfer;
CommandPool commandPoolCompute;

/* -- Render Objects -- */

FrameArray frame_array = { };

/* -- Global buffers -- */

buffer_partition_t global_staging_buffer_partition;
buffer_partition_t global_uniform_buffer_partition;
buffer_partition_t global_storage_buffer_partition;
buffer_partition_t global_draw_data_buffer_partition;

/* -- Function Definitions -- */

static void create_window_surface(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating window surface...");
	VkResult result = glfwCreateWindowSurface(vulkan_instance.handle, get_application_window(), nullptr, &surface);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_FATAL, "Window surface creation failed (error code: %i).", result);
		// TODO - do not use exit() here.
		exit(1);
	}
}

static void create_global_staging_buffer(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating global staging buffer...");

	const buffer_partition_create_info_t buffer_partition_create_info = {
		.physical_device = physical_device.handle,
		.device = device,
		.buffer_type = BUFFER_TYPE_STAGING,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = nullptr,
		.num_partition_sizes = 3,
		.partition_sizes = (VkDeviceSize[3]){
			5120,	// Render object mesh data--vertices
			768,	// Render object mesh data--indices
			262144	// Loaded image data
		}
	};
	
	global_staging_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

static void create_global_uniform_buffer(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating global uniform buffer...");

	const buffer_partition_create_info_t buffer_partition_create_info = {
		.physical_device = physical_device.handle,
		.device = device,
		.buffer_type = BUFFER_TYPE_UNIFORM,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = nullptr,
		.num_partition_sizes = 4,
		.partition_sizes = (VkDeviceSize[4]){
			2096,	// Compute matrices
			5120,	// Compute room texture
			8200,	// Compute area mesh
			1812	// Lighting data
		}
	};
	
	global_uniform_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

static void create_global_storage_buffer(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating global storage buffer...");

	const buffer_partition_create_info_t buffer_partition_create_info = {
		.physical_device = physical_device.handle,
		.device = device,
		.buffer_type = BUFFER_TYPE_STORAGE,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = nullptr,
		.num_partition_sizes = 3,
		.partition_sizes = (VkDeviceSize[3]){
			4224,	// Compute matrices
			81920,	// Area mesh data--vertices
			12288	// Area mesh data--indices
		}
	};
	
	global_storage_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

static void create_global_draw_data_buffer(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating global draw data buffer...");

	const buffer_partition_create_info_t buffer_partition_create_info = {
		.physical_device = physical_device.handle,
		.device = device,
		.buffer_type = BUFFER_TYPE_DRAW_DATA,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = nullptr,
		.num_partition_sizes = 2,
		.partition_sizes = (VkDeviceSize[2]){
			4,			// Indirect draw count
			68 * 7 * 4	// Indirect draw data
		}
	};
	
	global_draw_data_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

void create_vulkan_objects(void) {
	logMsg(loggerVulkan, LOG_LEVEL_INFO, "Initializing Vulkan...");

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
	create_global_draw_data_buffer();

	vkGetDeviceQueue(device, *physical_device.queue_family_indices.graphics_family_ptr, 0, &queueGraphics);
	vkGetDeviceQueue(device, *physical_device.queue_family_indices.present_family_ptr, 0, &queuePresent);
	vkGetDeviceQueue(device, *physical_device.queue_family_indices.transfer_family_ptr, 0, &queueTransfer);
	vkGetDeviceQueue(device, *physical_device.queue_family_indices.compute_family_ptr, 0, &queueCompute);

	commandPoolGraphics = createCommandPool(device, *physical_device.queue_family_indices.graphics_family_ptr, false, true);
	commandPoolTransfer = createCommandPool(device, *physical_device.queue_family_indices.transfer_family_ptr, true, true);
	commandPoolCompute = createCommandPool(device, *physical_device.queue_family_indices.compute_family_ptr, true, false);

	swapchain = create_swapchain(get_application_window(), surface, physical_device, device, VK_NULL_HANDLE);
	
	renderPass = createRenderPass(device, swapchain.image_format);
	
	shader_module_t vertex_shader_module = create_shader_module(device, VERTEX_SHADER_NAME);
	shader_module_t fragment_shader_module = create_shader_module(device, FRAGMENT_SHADER_NAME);
	
	graphics_pipeline = create_graphics_pipeline(device, swapchain, renderPass, graphics_descriptor_set_layout, vertex_shader_module.module_handle, fragment_shader_module.module_handle);
	
	destroy_shader_module(&vertex_shader_module);
	destroy_shader_module(&fragment_shader_module);
	
	create_framebuffers(device, renderPass, &swapchain);

	create_sampler(physical_device, device, &imageSamplerDefault);
	
	const FrameArrayCreateInfo frameArrayCreateInfo = {
		.num_frames = 2,
		.physical_device = physical_device,
		.vkDevice = device,
		.commandPool = commandPoolGraphics,
		.descriptorPool = (DescriptorPool){
			.handle = graphics_pipeline.descriptor_pool,
			.layout = graphics_pipeline.descriptor_set_layout,
			.vkDevice = device
		}
	};
	frame_array = createFrameArray(frameArrayCreateInfo);
}

void destroy_vulkan_objects(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Destroying Vulkan objects...");

	deleteFrameArray(&frame_array);

	vkDestroySampler(device, imageSamplerDefault, nullptr);

	destroy_graphics_pipeline(device, graphics_pipeline);
	vkDestroyRenderPass(device, renderPass, nullptr);
	destroy_swapchain(device, swapchain);
	
	deleteCommandPool(&commandPoolGraphics);
	deleteCommandPool(&commandPoolTransfer);
	deleteCommandPool(&commandPoolCompute);

	destroy_buffer_partition(&global_staging_buffer_partition);
	destroy_buffer_partition(&global_uniform_buffer_partition);
	destroy_buffer_partition(&global_storage_buffer_partition);
	destroy_buffer_partition(&global_draw_data_buffer_partition);

	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(vulkan_instance.handle, surface, nullptr);
	destroy_debug_messenger(vulkan_instance.handle, debug_messenger);
	destroy_vulkan_instance(vulkan_instance);
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Done destroying Vulkan objects.");
}
