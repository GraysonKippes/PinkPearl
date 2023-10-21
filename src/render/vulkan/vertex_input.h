#ifndef VERTEX_INPUT_H
#define VERTEX_INPUT_H

#include <stddef.h>

#include <vulkan/vulkan.h>

#define VERTEX_INPUT_NUM_ATTRIBUTES	3

#define VERTEX_INPUT_ELEMENT_STRIDE	8

extern const size_t num_vertices_per_rect;

extern const size_t num_indices_per_rect;

VkVertexInputBindingDescription get_binding_description(void);

// Returns a pointer-array containing a description for each attribute.
// Make sure to free the pointer after use.
VkVertexInputAttributeDescription *get_attribute_descriptions(void);

#endif	// VERTEX_INPUT_H
