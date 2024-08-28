#ifndef VERTEX_INPUT_H
#define VERTEX_INPUT_H

#include <stdint.h>

#define NUM_VERTICES_PER_QUAD 		4
#define VERTEX_INPUT_NUM_ATTRIBUTES	3
#define VERTEX_INPUT_ELEMENT_STRIDE	8

extern const uint32_t vertex_input_num_attributes;
extern const uint32_t vertex_input_element_stride;

// TODO - rename rect to quad.
extern const uint32_t num_vertices_per_rect;
extern const uint32_t num_indices_per_rect;

#endif	// VERTEX_INPUT_H
