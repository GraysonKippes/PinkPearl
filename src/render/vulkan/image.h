#ifndef IMAGE_H
#define IMAGE_H

#include <vulkan/vulkan.h>

#include "physical_device.h"

// TODO - move this to Texture or somewhere else, also make it return the sampler directly.
void create_sampler(physical_device_t physicalDevice, VkDevice vkDevice, VkSampler *pSampler);

#endif	// IMAGE_H
