#ifndef LOGICAL_DEVICE_H
#define LOGICAL_DEVICE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "physical_device.h"

void create_logical_device(physical_device_t physical_device, VkDevice *logical_device_ptr);

void submit_graphics_queue(VkQueue queue, VkCommandBuffer *command_buffer_ptr, VkSemaphore *wait_semaphore_ptr, VkSemaphore *signal_semaphore, VkFence in_flight_fence);

#endif	// LOGICAL_DEVICE_H
