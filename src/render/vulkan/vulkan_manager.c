#include "vulkan_manager.h"

#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "debug.h"
#include "log/log_stack.h"
#include "log/logging.h"
#include "glfw/glfw_manager.h"
#include "render/render_config.h"
#include "render/stb/image_data.h"
#include "util/bit.h"
#include "util/byte.h"

#include "command_buffer.h"
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
graphics_pipeline_t graphics_pipeline = { };
VkSampler imageSamplerDefault = VK_NULL_HANDLE;

/* -- Queues -- */

VkQueue queueGraphics;
VkQueue present_queue;
VkQueue transfer_queue;
VkQueue compute_queue;

/* -- Command Pools -- */

VkCommandPool cmdPoolGraphics;
VkCommandPool cmdPoolTransfer;
VkCommandPool compute_command_pool;

/* -- Render Objects -- */

static VkClearValue clear_color;
FrameArray frame_array = { };

/* -- Global buffers -- */

buffer_partition_t global_staging_buffer_partition;
buffer_partition_t global_uniform_buffer_partition;
buffer_partition_t global_storage_buffer_partition;
buffer_partition_t global_draw_data_buffer_partition;

/* -- Depth Image Attachment -- */

Image depthImage = { };
VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;

/* -- Function Definitions -- */

static void create_window_surface(void) {
	logMsg(VERBOSE, "Creating window surface...");
	VkResult result = glfwCreateWindowSurface(vulkan_instance.handle, get_application_window(), nullptr, &surface);
	if (result != VK_SUCCESS) {
		logMsgF(FATAL, "Window surface creation failed. (Error code: %i)", result);
		// TODO - do not use exit() here.
		exit(1);
	}
}

static void create_global_staging_buffer(void) {
	logMsg(VERBOSE, "Creating global staging buffer...");

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
	logMsg(VERBOSE, "Creating global uniform buffer...");

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
			2560,	// Compute room texture
			8200,	// Compute area mesh
			1812	// Lighting data
		}
	};
	
	global_uniform_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

static void create_global_storage_buffer(void) {
	logMsg(VERBOSE, "Creating global storage buffer...");

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
	logMsg(VERBOSE, "Creating global draw data buffer...");

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

static void createDepthImage(void) {
	
	const VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_D32_SFLOAT,
		.extent.width = swapchain.extent.width,
		.extent.height = swapchain.extent.height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	const VkResult imageCreateResult = vkCreateImage(device, &imageCreateInfo, nullptr, &depthImage.vkImage);
	if (imageCreateResult != VK_SUCCESS) {
		logMsgF(ERROR, "Error creating depth image: image creation failed (error code: %i).", imageCreateResult);
		return;
	}
	
	
}

void create_vulkan_objects(void) {
	log_stack_push("Vulkan");
	logMsg(INFO, "Initializing Vulkan...");

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
	vkGetDeviceQueue(device, *physical_device.queue_family_indices.present_family_ptr, 0, &present_queue);
	vkGetDeviceQueue(device, *physical_device.queue_family_indices.transfer_family_ptr, 0, &transfer_queue);
	vkGetDeviceQueue(device, *physical_device.queue_family_indices.compute_family_ptr, 0, &compute_queue);

	create_command_pool(device, default_command_pool_flags, *physical_device.queue_family_indices.graphics_family_ptr, &cmdPoolGraphics);
	create_command_pool(device, transfer_command_pool_flags, *physical_device.queue_family_indices.transfer_family_ptr, &cmdPoolTransfer);
	create_command_pool(device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, *physical_device.queue_family_indices.compute_family_ptr, &compute_command_pool);

	clear_color = (VkClearValue){ { { 0.0F, 0.0F, 0.0F, 1.0F } } };

	swapchain = create_swapchain(get_application_window(), surface, physical_device, device, VK_NULL_HANDLE);
	shader_module_t vertex_shader_module = create_shader_module(device, VERTEX_SHADER_NAME);
	shader_module_t fragment_shader_module = create_shader_module(device, FRAGMENT_SHADER_NAME);
	graphics_pipeline = create_graphics_pipeline(device, swapchain, graphics_descriptor_set_layout, vertex_shader_module.module_handle, fragment_shader_module.module_handle);
	create_framebuffers(device, graphics_pipeline.render_pass, &swapchain);

	destroy_shader_module(&vertex_shader_module);
	destroy_shader_module(&fragment_shader_module);

	create_sampler(physical_device, device, &imageSamplerDefault);
	
	const frame_array_create_info_t frame_array_create_info = {
		.num_frames = 2,
		.physical_device = physical_device,
		.device = device,
		.command_pool = cmdPoolGraphics,
		.descriptor_pool = graphics_pipeline.descriptor_pool,
		.descriptor_set_layout = graphics_pipeline.descriptor_set_layout
	};
	frame_array = create_frame_array(frame_array_create_info);
	
	log_stack_pop();
}

void destroy_vulkan_objects(void) {
	log_stack_push("Vulkan");
	logMsg(VERBOSE, "Destroying Vulkan objects...");

	destroy_frame_array(&frame_array);

	vkDestroySampler(device, imageSamplerDefault, nullptr);

	destroy_graphics_pipeline(device, graphics_pipeline);
	destroy_swapchain(device, swapchain);

	vkDestroyCommandPool(device, cmdPoolGraphics, nullptr);
	vkDestroyCommandPool(device, cmdPoolTransfer, nullptr);
	vkDestroyCommandPool(device, compute_command_pool, nullptr);

	destroy_buffer_partition(&global_staging_buffer_partition);
	destroy_buffer_partition(&global_uniform_buffer_partition);
	destroy_buffer_partition(&global_storage_buffer_partition);
	destroy_buffer_partition(&global_draw_data_buffer_partition);

	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(vulkan_instance.handle, surface, nullptr);
	destroy_debug_messenger(vulkan_instance.handle, debug_messenger);
	destroy_vulkan_instance(vulkan_instance);
	
	logMsg(VERBOSE, "Done destroying Vulkan objects.");
	log_stack_pop();
}
