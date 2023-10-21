#include "vertex_input.h"

#include <stdlib.h>

const size_t num_vertices_per_rect = 4;

const size_t num_indices_per_rect = 6;

static const size_t attribute_sizes[] = { 3, 2, 3 };

VkVertexInputBindingDescription get_binding_description(void) {

	VkVertexInputBindingDescription binding;
	binding.binding = 0;
	binding.stride = VERTEX_INPUT_ELEMENT_STRIDE * sizeof(float);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return binding;
}

VkFormat size_to_format(size_t size) {
	switch (size) {
	case 1: return VK_FORMAT_R32_SFLOAT;
	case 2: return VK_FORMAT_R32G32_SFLOAT;
	case 3: return VK_FORMAT_R32G32B32_SFLOAT;
	case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
	default: return VK_FORMAT_UNDEFINED;
	}
}

VkVertexInputAttributeDescription *get_attribute_descriptions(void) {

	VkVertexInputAttributeDescription *attributes = calloc(VERTEX_INPUT_NUM_ATTRIBUTES, sizeof(VkVertexInputAttributeDescription));

	uint32_t offset = 0;
	for (uint32_t i = 0; i < VERTEX_INPUT_NUM_ATTRIBUTES; ++i) {

		attributes[i].binding = 0;
		attributes[i].location = i;
		attributes[i].format = size_to_format(attribute_sizes[i]);
		attributes[i].offset = offset;

		offset += attribute_sizes[i] * sizeof(float);
	}

	return attributes;
}
