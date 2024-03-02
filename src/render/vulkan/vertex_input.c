#include "vertex_input.h"

#include <stdlib.h>

const uint32_t vertex_input_num_attributes = VERTEX_INPUT_NUM_ATTRIBUTES;
const uint32_t vertex_input_element_stride = VERTEX_INPUT_ELEMENT_STRIDE;
const uint32_t num_vertices_per_rect = 4;
const uint32_t num_indices_per_rect = 6;

static const uint32_t attribute_sizes[VERTEX_INPUT_NUM_ATTRIBUTES] = { 3, 2, 3 };

VkVertexInputBindingDescription get_binding_description(void) {
	return (VkVertexInputBindingDescription){
		.binding = 0,
		.stride = vertex_input_element_stride * sizeof(float),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};
}

VkFormat size_to_format(uint32_t size) {
	switch (size) {
		case 1: return VK_FORMAT_R32_SFLOAT;
		case 2: return VK_FORMAT_R32G32_SFLOAT;
		case 3: return VK_FORMAT_R32G32B32_SFLOAT;
		case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	return VK_FORMAT_UNDEFINED;
}

void get_attribute_descriptions(VkVertexInputAttributeDescription attribute_descriptions[VERTEX_INPUT_NUM_ATTRIBUTES]) {
	uint32_t offset = 0;
	for (uint32_t i = 0; i < vertex_input_num_attributes; ++i) {

		attribute_descriptions[i].binding = 0;
		attribute_descriptions[i].location = i;
		attribute_descriptions[i].format = size_to_format(attribute_sizes[i]);
		attribute_descriptions[i].offset = offset;

		offset += attribute_sizes[i] * sizeof(float);
	}
}
