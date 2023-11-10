#include "model.h"

#include <stdlib.h>
#include <string.h>



#define NUM_PREMADE_MODELS 2

static model_t premade_models[NUM_PREMADE_MODELS];



const size_t model_index_one_by_one = 0;

const size_t model_index_one_by_one_point_five = 1;



// Model vertices for 8x5 room.
static const float model_room0_vertices[20] = {
	// Positions		Texture	
	-4.0F, -2.5F, 0.0F,	0.0F, 0.0F,	// Top-left
	4.0F, -2.5F, 0.0F,	1.0F, 0.0F,	// Top-right
	4.0F, 2.5F, 0.0F,	1.0F, 1.0F,	// Bottom-right
	-4.0F, 2.5F, 0.0F,	0.0F, 1.0F	// Bottom-left
};

// Model vertices for 16x10 room.
static const float model_room1_vertices[20] = {
	// Positions		Texture	
	-8.0F, -5.0F, 0.0F,	0.0F, 0.0F,
	8.0F, -5.0F, 0.0F,	0.0F, 0.0F,
	8.0F, 5.0F, 0.0F,	0.0F, 0.0F,
	-8.0F, 5.0F, 0.0F,	0.0F, 0.0F
};

static const float model0_vertices[32] = {
	// Positions		Texture		Color
	-0.5F, -0.5F, 0.0F,	0.0F, 0.0F,	1.0F, 1.0F, 1.0F, // 0
	0.5F, -0.5F, 0.0F,	0.0F, 0.0F,	1.0F, 1.0F, 1.0F, // 1
	0.5F, 0.5F, 0.0F,	0.0F, 0.0F,	1.0F, 1.0F, 1.0F, // 2
	-0.5F, 0.5F, 0.0F,	0.0F, 0.0F,	1.0F, 1.0F, 1.0F, // 3
};

static const index_t model_indices[6] = {
	0, 1, 2, // Triangle 0
	2, 3, 0	 // Triangle 1
};

static const index_t model0_indices[6] = {
	0, 1, 2, // Triangle 0
	2, 3, 0	 // Triangle 1
};



model_t create_model(uint32_t num_vertices, const float *vertices, uint32_t num_indices, index_t *indices) {

	model_t model;

	model.m_num_vertices = num_vertices;
	model.m_vertices = malloc(num_vertices * sizeof(float));
	if (model.m_vertices != NULL)
		memcpy(model.m_vertices, vertices, num_vertices * sizeof(float));

	model.m_num_indices = num_indices;
	model.m_indices = malloc(num_indices * sizeof(index_t));
	if (model.m_indices != NULL)
		memcpy(model.m_indices, indices, num_indices * sizeof(index_t));

	return model;
}

void free_model(model_t model) {
	free(model.m_vertices);
	free(model.m_indices);
}

void make_premade_models(void) {

	premade_models[0] = create_model(20, model_room0_vertices, 6, model_indices);
	premade_models[1] = create_model(20, model_room1_vertices, 6, model_indices);
}

void free_premade_models(void) {
	for (size_t i = 0; i < NUM_PREMADE_MODELS; ++i) {
		free_model(premade_models[i]);
	}
}

model_t get_premade_model(size_t index) {
	return premade_models[index];
}
