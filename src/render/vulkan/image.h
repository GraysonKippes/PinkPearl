#ifndef IMAGE_H
#define IMAGE_H

#include <vulkan/vulkan.h>

#include "physical_device.h"

void create_sampler(physical_device_t physical_device, VkDevice device, VkSampler *sampler_ptr);

#endif	// IMAGE_H
