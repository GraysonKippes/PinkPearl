#ifndef LOGICAL_DEVICE_H
#define LOGICAL_DEVICE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "physical_device.h"

void create_logical_device(physical_device_t physical_device, VkDevice *logical_device_ptr);

#endif	// LOGICAL_DEVICE_H
