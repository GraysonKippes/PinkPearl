#ifndef RENDER_VECTOR_H
#define RENDER_VECTOR_H

#include "vector4F.h"

typedef struct RenderVector {
	vector4F_t current;
	vector4F_t previous;
} RenderVector;

typedef struct RenderTransform {
	RenderVector translation;
	RenderVector scaling;
	RenderVector rotation;
} RenderTransform;

// Updates the render vector, setting the previous vector to what the vector was before the update.
void render_vector_set(RenderVector *const render_vector_ptr, const vector4F_t new_vector);

// Force-updates both the current vector and the previous vector to the new vector.
void render_vector_reset(RenderVector *const render_vector_ptr, const vector4F_t new_vector);

// Sets the previous vector to the current vector.
void render_vector_settle(RenderVector *const render_vector_ptr);

#endif	// RENDER_VECTOR_H
