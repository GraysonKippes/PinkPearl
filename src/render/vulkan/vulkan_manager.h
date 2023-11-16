#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include <vulkan/vulkan.h>

#include "game/area/room.h"
#include "render/color.h"
#include "render/model.h"
#include "render/projection.h"
#include "render/render_object.h"

#include "compute_pipeline.h"
#include "frame.h"
#include "graphics_pipeline.h"
#include "memory.h"
#include "physical_device.h"
#include "swapchain.h"

// This file contains all the vulkan objects and functions to create them and destroy them.



/* -- Core State -- */

extern VkSurfaceKHR surface;

extern physical_device_t physical_device;

extern memory_type_set_t memory_type_set;

extern VkDevice device;

extern swapchain_t swapchain;

extern graphics_pipeline_t graphics_pipeline;

extern VkSampler sampler_default;

/* -- Compute -- */

extern compute_pipeline_t compute_pipeline_matrices;
extern compute_pipeline_t compute_pipeline_room_texture;
extern compute_pipeline_t compute_pipeline_lighting;

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
extern VkBuffer global_staging_buffer;
extern VkDeviceMemory global_staging_memory;
extern void *global_staging_mapped_memory;

// Used for uniform data into both compute shaders and graphics (vertex, fragment) shaders.
extern VkBuffer global_uniform_buffer;
extern const VkDeviceSize global_uniform_buffer_size;
extern VkDeviceMemory global_uniform_memory;



#define MAX_FRAMES_IN_FLIGHT	2

extern frame_t frames[MAX_FRAMES_IN_FLIGHT];

extern size_t current_frame;

#define FRAME (frames[current_frame])







// Creates all Vulkan objects needed for the rendering system.
void create_vulkan_objects(void);

// Destroys the Vulkan objects created for the rendering system.
void destroy_vulkan_objects(void);

#endif	// VULKAN_MANAGER_H
