#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "compute_pipeline.h"
#include "frame.h"
#include "graphics_pipeline.h"
#include "memory.h"
#include "physical_device.h"
#include "swapchain.h"

/* -- Core State -- */

extern VkSurfaceKHR surface;
extern physical_device_t physical_device;
extern memory_type_set_t memory_type_set;
extern VkDevice device;
extern swapchain_t swapchain;
extern graphics_pipeline_t graphics_pipeline;
extern VkSampler imageSamplerDefault;

/* -- Queues -- */

extern VkQueue graphics_queue;
extern VkQueue present_queue;
extern VkQueue transfer_queue;
extern VkQueue compute_queue;

/* -- Command Pools -- */

extern VkCommandPool render_command_pool;
extern VkCommandPool transfer_command_pool;
extern VkCommandPool compute_command_pool;

/* -- Global buffers -- */

// Used for staging data to GPU-only buffers (storage, vertex, index) and images.
extern buffer_partition_t global_staging_buffer_partition;

// Used for uniform data into both compute shaders and graphics (vertex, fragment) shaders.
extern buffer_partition_t global_uniform_buffer_partition;

// Used for GPU-only bulk storage data.
extern buffer_partition_t global_storage_buffer_partition;

extern buffer_partition_t global_draw_data_buffer_partition;

extern frame_array_t frame_array;

// Creates all Vulkan objects needed for the rendering system.
void create_vulkan_objects(void);

// Destroys the Vulkan objects created for the rendering system.
void destroy_vulkan_objects(void);

#endif	// VULKAN_MANAGER_H
