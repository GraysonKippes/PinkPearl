#include "model.h"

#include "log/logging.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>



#define NUM_PREMADE_MODELS 1

static const uint32_t num_premade_models = NUM_PREMADE_MODELS;

static model_t premade_models[NUM_PREMADE_MODELS];



const size_t model_index_one_by_one = 0;

const size_t model_index_one_by_one_point_five = 1;



static const float model0_vertices[20] = {
	// Positions		Texture
	-0.5F, -1.0F, 0.0F,	0.0F, 0.0F,	// Top-left
	0.5F, -1.0F, 0.0F,	1.0F, 0.0F,	// Top-right
	0.5F, 0.5F, 0.0F,	1.0F, 1.0F,	// Bottom-right
	-0.5F, 0.5F, 0.0F,	0.0F, 1.0F	// Bottom-left
};

static const index_t model_indices[6] = {
	0, 1, 2, // Triangle 0
	2, 3, 0	 // Triangle 1
};



model_t create_model(uint32_t num_vertices, const float *vertices, uint32_t num_indices, const index_t *indices) {

	model_t model;

	model.num_vertices = num_vertices;
	model.vertices = malloc(num_vertices * sizeof(float));
	if (model.vertices != NULL) {
		memcpy(model.vertices, vertices, num_vertices * sizeof(float)); 
	}

	model.num_indices = num_indices;
	model.indices = malloc(num_indices * sizeof(index_t));
	if (model.indices != NULL) {
		memcpy(model.indices, indices, num_indices * sizeof(index_t));
	}

	return model;
}

void free_model(model_t model) {
	free(model.vertices);
	free(model.indices);
}

void load_premade_models(void) {
	premade_models[0] = create_model(20, model0_vertices, 6, model_indices);
}

void free_premade_models(void) {
	for (size_t i = 0; i < NUM_PREMADE_MODELS; ++i) {
		free_model(premade_models[i]);
	}
}

model_t get_premade_model(uint32_t model_index) {

	if (model_index >= num_premade_models) {
		logf_message(ERROR, "Error getting premade model: model index (%u) exceeds number of premade models (%u).", model_index, num_premade_models);
		return (model_t){ 0 };
	}

	return premade_models[model_index];
}
