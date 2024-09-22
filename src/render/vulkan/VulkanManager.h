#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "Buffer2.h"
#include "Draw.h"
#include "frame.h"
#include "memory.h"
#include "physical_device.h"
#include "Pipeline.h"
#include "Swapchain.h"
#include "texture.h"
#include "vulkan_instance.h"
#include "math/projection.h"

#include "render/render_config.h"

/* -- Vulkan Module Configuration -- */

#define VK_CONF_MAX_NUM_QUADS (NUM_RENDER_OBJECT_SLOTS * MAX_NUM_RENDER_OBJECT_QUADS)

extern const int vkConfMaxNumQuads;

/* -- Core State -- */

extern PhysicalDevice physical_device;
extern MemoryTypeIndexSet memory_type_set;
extern VkDevice device;

/* -- Queues -- */

extern VkQueue queueGraphics;
extern VkQueue queuePresent;
extern VkQueue queueTransfer;
extern VkQueue queueCompute;

/* -- Command Pools -- */

extern CommandPool commandPoolGraphics;
extern CommandPool commandPoolTransfer;
extern CommandPool commandPoolCompute;

extern ModelPool modelPoolMain;

extern ModelPool modelPoolDebug;

/* -- Global buffers -- */

// Used for staging data to GPU-only buffers (storage, vertex, index) and images.
extern BufferPartition global_staging_buffer_partition;

// Used for uniform data into both compute shaders and graphics (vertex, fragment) shaders.
extern BufferPartition global_uniform_buffer_partition;

// Used for GPU-only bulk storage data.
extern BufferPartition global_storage_buffer_partition;

extern FrameArray frame_array;

// Creates all Vulkan objects needed for the rendering system.
void initVulkanManager(void);

// Destroys the Vulkan objects created for the rendering system.
void terminateVulkanManager(void);

void createTestDebugModel();

void drawFrame(const float deltaTime, const Vector4F cameraPosition, const ProjectionBounds projectionBounds);

#endif	// VULKAN_MANAGER_H
