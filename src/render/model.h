#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>

// Type-definition for the integer type used for index data.
typedef uint16_t index_t;

typedef struct model_t {

	// TODO - rename to `num_vertex_elements`.
	uint32_t num_vertices;

	// TODO - rename to `vertex_elements`.
	float *vertices;

	uint32_t num_indices;

	uint16_t *indices;

} model_t;

model_t create_model(uint32_t num_vertices, const float *vertices, uint32_t num_indices, index_t *indices);

void free_model(model_t model);

void load_premade_models(void);

void free_premade_models(void);

model_t get_premade_model(uint32_t index);

#endif	// MODEL_H
