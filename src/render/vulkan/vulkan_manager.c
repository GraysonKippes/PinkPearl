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
#include "util/Types.h"

#include "CommandBuffer.h"
#include "descriptor.h"
#include "GraphicsPipeline.h"
#include "logical_device.h"
#include "queue.h"
#include "Shader.h"
#include "texture_manager.h"
#include "vertex_input.h"
#include "compute/compute_matrices.h"
#include "compute/compute_room_texture.h"

/* -- Vulkan Module Configuration -- */

const int vkConfMaxNumQuads = VK_CONF_MAX_NUM_QUADS;

/* -- Vulkan Objects -- */

static VulkanInstance vulkan_instance = { };

static VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

WindowSurface windowSurface = { };

PhysicalDevice physical_device = { };

memory_type_set_t memory_type_set = { };

VkDevice device = VK_NULL_HANDLE;

Swapchain swapchain = { };

Pipeline graphicsPipeline = { };

Pipeline graphicsPipelineDebug = { };

Sampler samplerDefault = { };

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

ModelPool modelPoolMain = nullptr;

/* -- Graphics Pipeline Descriptor Set Layout -- */

static const DescriptorBinding graphicsPipelineDescriptorBindings[5] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },	// Draw data
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_VERTEX_BIT },	// Matrix buffer
	{ .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .count = VK_CONF_MAX_NUM_QUADS, .stages = VK_SHADER_STAGE_FRAGMENT_BIT },	// Texture array
	{ .type = VK_DESCRIPTOR_TYPE_SAMPLER, .count = 1, .stages = VK_SHADER_STAGE_FRAGMENT_BIT },	// Sampler
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_FRAGMENT_BIT }	// Lighting
};

static const DescriptorSetLayout graphicsPipelineDescriptorSetLayout = {
	.num_bindings = 5,
	.bindings = (DescriptorBinding *const)graphicsPipelineDescriptorBindings
};

/* -- Function Definitions -- */

static void create_global_staging_buffer(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating global staging buffer...");

	const buffer_partition_create_info_t buffer_partition_create_info = {
		.physical_device = physical_device.vkPhysicalDevice,
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
		.physical_device = physical_device.vkPhysicalDevice,
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
		.physical_device = physical_device.vkPhysicalDevice,
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
		.physical_device = physical_device.vkPhysicalDevice,
		.device = device,
		.buffer_type = BUFFER_TYPE_DRAW_DATA,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = nullptr,
		.num_partition_sizes = 2,
		.partition_sizes = (VkDeviceSize[2]){
			4,			// Indirect draw count
			256 * 7 * 4	// Indirect draw data
		}
	};
	
	global_draw_data_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

void create_vulkan_objects(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Initializing Vulkan...");

	vulkan_instance = create_vulkan_instance();

	if (debug_enabled) {
		setup_debug_messenger(vulkan_instance.handle, &debug_messenger);
	}

	windowSurface = createWindowSurface(vulkan_instance);

	physical_device = select_physical_device(vulkan_instance, windowSurface);
	memory_type_set = select_memory_types(physical_device.vkPhysicalDevice);

	create_device(vulkan_instance, physical_device, &device);

	create_global_staging_buffer();
	create_global_uniform_buffer();
	create_global_storage_buffer();
	create_global_draw_data_buffer();

	vkGetDeviceQueue(device, *physical_device.queueFamilyIndices.graphics_family_ptr, 0, &queueGraphics);
	vkGetDeviceQueue(device, *physical_device.queueFamilyIndices.present_family_ptr, 0, &queuePresent);
	vkGetDeviceQueue(device, *physical_device.queueFamilyIndices.transfer_family_ptr, 0, &queueTransfer);
	vkGetDeviceQueue(device, *physical_device.queueFamilyIndices.compute_family_ptr, 0, &queueCompute);

	commandPoolGraphics = createCommandPool(device, *physical_device.queueFamilyIndices.graphics_family_ptr, false, true);
	commandPoolTransfer = createCommandPool(device, *physical_device.queueFamilyIndices.transfer_family_ptr, true, true);
	commandPoolCompute = createCommandPool(device, *physical_device.queueFamilyIndices.compute_family_ptr, true, false);

	swapchain = createSwapchain(get_application_window(), windowSurface, physical_device, device, VK_NULL_HANDLE);
	
	ShaderModule vertexShaderModule = createShaderModule(device, SHADER_STAGE_VERTEX, VERTEX_SHADER_NAME);
	ShaderModule fragmentShaderModule = createShaderModule(device, SHADER_STAGE_FRAGMENT, FRAGMENT_SHADER_NAME);
	
	graphicsPipeline = createGraphicsPipeline(device, swapchain, graphicsPipelineDescriptorSetLayout,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL,
			2, (ShaderModule[]){vertexShaderModule, fragmentShaderModule});
			
	graphicsPipelineDebug = createGraphicsPipeline(device, swapchain, graphicsPipelineDescriptorSetLayout,
			VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_POLYGON_MODE_FILL,
			2, (ShaderModule[]){vertexShaderModule, fragmentShaderModule});
	
	destroyShaderModule(&vertexShaderModule);
	destroyShaderModule(&fragmentShaderModule);

	samplerDefault = createSampler(device, physical_device);
	
	const FrameArrayCreateInfo frameArrayCreateInfo = {
		.num_frames = 2,
		.physical_device = physical_device,
		.vkDevice = device,
		.commandPool = commandPoolGraphics,
		.descriptorPool = (DescriptorPool){
			.handle = graphicsPipeline.vkDescriptorPool,
			.layout = graphicsPipeline.vkDescriptorSetLayout,
			.vkDevice = device
		}
	};
	frame_array = createFrameArray(frameArrayCreateInfo);
	
	init_compute_matrices(device);
	init_compute_room_texture(device);
	
	createModelPool(0, 4, 0, 6, 256, &modelPoolMain);
}

void destroy_vulkan_objects(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Destroying Vulkan objects...");

	vkDeviceWaitIdle(device);

	terminate_compute_matrices();
	terminate_compute_room_texture();
	terminateTextureManager();

	deleteModelPool(&modelPoolMain);

	deleteFrameArray(&frame_array);
	
	deleteSampler(&samplerDefault);

	deletePipeline(&graphicsPipeline);
	deletePipeline(&graphicsPipelineDebug);
	
	deleteSwapchain(&swapchain);
	
	deleteCommandPool(&commandPoolGraphics);
	deleteCommandPool(&commandPoolTransfer);
	deleteCommandPool(&commandPoolCompute);

	destroy_buffer_partition(&global_staging_buffer_partition);
	destroy_buffer_partition(&global_uniform_buffer_partition);
	destroy_buffer_partition(&global_storage_buffer_partition);
	destroy_buffer_partition(&global_draw_data_buffer_partition);

	vkDestroyDevice(device, nullptr);
	
	deleteWindowSurface(&windowSurface);
	
	deletePhysicalDevice(&physical_device);
	
	destroy_debug_messenger(vulkan_instance.handle, debug_messenger);
	destroy_vulkan_instance(vulkan_instance);
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Destroyed Vulkan objects.");
}
