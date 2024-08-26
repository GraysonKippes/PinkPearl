#ifndef LOGICAL_DEVICE_H
#define LOGICAL_DEVICE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "physical_device.h"
#include "vulkan_instance.h"

#define NUM_QUEUES 4

void create_device(VulkanInstance vulkan_instance, PhysicalDevice physical_device, VkDevice *device_ptr);

#endif	// LOGICAL_DEVICE_H
