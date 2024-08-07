#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "ComputePipeline.h"
#include "frame.h"
#include "GraphicsPipeline.h"
#include "memory.h"
#include "physical_device.h"
#include "swapchain.h"
#include "texture.h"

#include "render/render_config.h"

/* -- Vulkan Module Configuration -- */

#define VK_CONF_MAX_NUM_QUADS (NUM_RENDER_OBJECT_SLOTS * MAX_NUM_RENDER_OBJECT_QUADS)

extern const int vkConfMaxNumQuads;

/* -- Core State -- */

extern VkSurfaceKHR surface;
extern physical_device_t physical_device;
extern memory_type_set_t memory_type_set;
extern VkDevice device;
extern Swapchain swapchain;
extern GraphicsPipeline graphics_pipeline;
extern VkRenderPass renderPass;
extern VkSampler imageSamplerDefault;

/* -- Queues -- */

extern VkQueue queueGraphics;
extern VkQueue queuePresent;
extern VkQueue queueTransfer;
extern VkQueue queueCompute;

/* -- Command Pools -- */

extern CommandPool commandPoolGraphics;
extern CommandPool commandPoolTransfer;
extern CommandPool commandPoolCompute;

/* -- Global buffers -- */

// Used for staging data to GPU-only buffers (storage, vertex, index) and images.
extern buffer_partition_t global_staging_buffer_partition;

// Used for uniform data into both compute shaders and graphics (vertex, fragment) shaders.
extern buffer_partition_t global_uniform_buffer_partition;

// Used for GPU-only bulk storage data.
extern buffer_partition_t global_storage_buffer_partition;

extern buffer_partition_t global_draw_data_buffer_partition;

extern FrameArray frame_array;

// Creates all Vulkan objects needed for the rendering system.
void create_vulkan_objects(void);

// Destroys the Vulkan objects created for the rendering system.
void destroy_vulkan_objects(void);

#endif	// VULKAN_MANAGER_H
