#include "image.h"

void create_sampler(physical_device_t physical_device, VkDevice device, VkSampler *sampler_ptr) {

	static const VkFilter filter = VK_FILTER_NEAREST;
	static const VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkSamplerCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.magFilter = filter;
	create_info.minFilter = filter;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	create_info.addressModeU = address_mode;
	create_info.addressModeV = address_mode;
	create_info.addressModeW = address_mode;
	create_info.mipLodBias = 0.0F;
	create_info.anisotropyEnable = VK_TRUE;
	create_info.maxAnisotropy = physical_device.properties.limits.maxSamplerAnisotropy;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	create_info.minLod = 0.0F;
	create_info.maxLod = 0.0F;
	create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	create_info.unnormalizedCoordinates = VK_FALSE;

	vkCreateSampler(device, &create_info, nullptr, sampler_ptr);
}

