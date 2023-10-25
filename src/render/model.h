#ifndef MODEL_H
#define MODEL_H

#include <stddef.h>
#include <stdint.h>

// Type-definition for the integer type used for index data.
typedef uint16_t index_t;

typedef struct model_t {

	uint32_t m_num_vertices;

	float *m_vertices;

	uint32_t m_num_indices;

	uint16_t *m_indices;

} model_t;

model_t create_model(uint32_t num_vertices, const float *vertices, uint32_t num_indices, index_t *indices);

void free_model(model_t model);

void make_premade_models(void);

void free_premade_models(void);

model_t get_premade_model(size_t index);

#endif	// MODEL_H
