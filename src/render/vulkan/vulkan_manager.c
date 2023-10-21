#include "vulkan_manager.h"

#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "debug.h"
#include "log/logging.h"
#include "glfw/glfw_manager.h"

#include "command_buffer.h"
#include "compute_pipeline.h"
#include "descriptor.h"
#include "graphics_pipeline.h"
#include "logical_device.h"
#include "physical_device.h"
#include "shader.h"
#include "swapchain.h"
#include "vulkan_instance.h"

/* -- Vulkan Objects -- */

static VkInstance vulkan_instance = VK_NULL_HANDLE;
static VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

static VkSurfaceKHR surface = VK_NULL_HANDLE;

static physical_device_t physical_device;

static VkDevice logical_device = VK_NULL_HANDLE;

static swapchain_t swapchain;

static graphics_pipeline_t graphics_pipeline;

/* -- Compute -- */

#define NUM_COMPUTE_PIPELINES 1

static VkPipeline compute_pipelines[NUM_COMPUTE_PIPELINES];



static VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;

static VkSampler default_sampler = VK_NULL_HANDLE;

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

typedef struct frame_objects_t {

	// Buffer to upload drawing commands for this frame to.
	VkCommandBuffer m_command_buffer;

	// Signaled when the image for this frame is available.
	VkSemaphore m_image_available_semaphore;

	// Signaled when this frame is done being rendered and can be displayed to the surface.
	VkSemaphore m_render_finished_semaphore;

	// Signaled when this frame is done.
	VkFence m_in_flight_fence;

} frame_objects_t;

#define MAX_FRAMES_IN_FLIGHT	2

static frame_objects_t frames[MAX_FRAMES_IN_FLIGHT];

static size_t current_frame = 0;

#define FRAME (frames[current_frame])

void create_frame_objects(void);



/* -- Function Definitions -- */

void create_vulkan_objects(void) {

	create_vulkan_instance(&vulkan_instance);
	if (debug_enabled)
		setup_debug_messenger(vulkan_instance, &debug_messenger);

	{
		log_message(INFO, "Creating window surface...");

		VkResult result = glfwCreateWindowSurface(vulkan_instance, get_application_window(), NULL, &surface);
		if (result != VK_SUCCESS) {
			logf_message(FATAL, "Window surface creation failed. (Error code: %i)", result);
			exit(1);
		}
	}

	physical_device = select_physical_device(vulkan_instance, surface);

	create_logical_device(physical_device, &logical_device);

	log_message(INFO, "Retrieving device queues...");

	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_graphics_family_ptr, 0, &graphics_queue);
	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_present_family_ptr, 0, &present_queue);
	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_transfer_family_ptr, 0, &transfer_queue);
	vkGetDeviceQueue(logical_device, *physical_device.m_queue_family_indices.m_compute_family_ptr, 0, &compute_queue);

	create_command_pool(logical_device, default_command_pool_flags, *physical_device.m_queue_family_indices.m_graphics_family_ptr, &render_command_pool);
	create_command_pool(logical_device, transfer_command_pool_flags, *physical_device.m_queue_family_indices.m_compute_family_ptr, &transfer_command_pool);

	swapchain = create_swapchain(get_application_window(), surface, physical_device, logical_device, VK_NULL_HANDLE);

	// TODO - create shaders
	//		- create pipeline - create render pass
	//		- destroy shaders
	
	VkShaderModule vertex_shader;
	create_shader_module(logical_device, "vertex_shader.spv", &vertex_shader);

	VkShaderModule fragment_shader;
	create_shader_module(logical_device, "fragment_shader.spv", &fragment_shader);

	VkDescriptorSetLayout graphics_pipeline_layout = VK_NULL_HANDLE;
	create_descriptor_set_layout(logical_device, graphics_descriptor_layout, &graphics_pipeline_layout);

	graphics_pipeline = create_graphics_pipeline(logical_device, swapchain, graphics_pipeline_layout, vertex_shader, fragment_shader);

	vkDestroyShaderModule(logical_device, vertex_shader, NULL);
	vkDestroyShaderModule(logical_device, fragment_shader, NULL);

	create_framebuffers(logical_device, graphics_pipeline.m_render_pass, &swapchain);

	const uint32_t num_compute_shaders = NUM_COMPUTE_PIPELINES;

	compute_shader_t compute_matrices;
	create_shader_module(logical_device, "compute_matrices.spv", &compute_matrices.m_module);
	create_descriptor_set_layout(logical_device, compute_matrices_layout, &compute_matrices.m_descriptor_set_layout);

	create_compute_pipelines(logical_device, compute_pipelines, num_compute_shaders, compute_matrices);

	vkDestroyShaderModule(logical_device, compute_matrices.m_module, NULL);

	create_frame_objects();
}

void create_frame_objects(void) {
	
	VkSemaphoreCreateInfo semaphore_info;
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info;
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkCreateSemaphore(logical_device, &semaphore_info, NULL, &frames[i].m_image_available_semaphore);
		vkCreateSemaphore(logical_device, &semaphore_info, NULL, &frames[i].m_render_finished_semaphore);
		vkCreateFence(logical_device, &fence_info, NULL, &frames[i].m_in_flight_fence);
	}
}

void destroy_vulkan_objects(void) {

	vkDeviceWaitIdle(logical_device);
}

static uint32_t image_index = 0;

void render_frame(double delta_time) {

	vkWaitForFences(logical_device, 1, &FRAME.m_in_flight_fence, VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(logical_device, swapchain.m_handle, UINT64_MAX, FRAME.m_image_available_semaphore, VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		// TODO - error handling
	}

	vkResetFences(logical_device, 1, &FRAME.m_in_flight_fence);
	vkResetCommandBuffer(FRAME.m_command_buffer, 0);

	//beginCommandBuffer(commandBuffers[currentFrame]);
	//beginRenderPass(commandBuffers[currentFrame], pipeline.renderPass, swapchain.framebuffers[imageIndex], swapchain.extent);
	//bindPipeline(commandBuffers[currentFrame], pipeline.handle);
	//vkCmdEndRenderPass(commandBuffers[currentFrame]);
	//endCommandBuffer(commandBuffers[currentFrame]);
	//submitGraphicsQueue((commandBuffers.data() + currentFrame), (imageAvailableSemaphores.data() + currentFrame), (renderFinishedSemaphores.data() + currentFrame), inFlightFences[currentFrame]);

	VkPresentInfoKHR present_info;
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &FRAME.m_render_finished_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain.m_handle;
	present_info.pImageIndices = &image_index;
	present_info.pResults = NULL;

	//vkQueuePresentKHR(present_queue, &present_info);

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}
