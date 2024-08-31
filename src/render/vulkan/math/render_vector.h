#ifndef RENDER_VECTOR_H
#define RENDER_VECTOR_H

#include "vector4F.h"

typedef struct RenderVector {
	Vector4F current;
	Vector4F previous;
} RenderVector;

typedef struct ModelTransform {
	RenderVector translation;
	RenderVector scaling;
	RenderVector rotation;
} ModelTransform;

// Updates the render vector, setting the previous vector to what the vector was before the update.
void renderVectorSet(RenderVector *const pRenderVector, const Vector4F newVector);

// Force-updates both the current vector and the previous vector to the new vector.
void renderVectorReset(RenderVector *const pRenderVector, const Vector4F newVector);

// Sets the previous vector to the current vector.
void renderVectorSettle(RenderVector *const pRenderVector);

ModelTransform makeModelTransform(const Vector4F translation, const Vector4F scaling, const Vector4F rotation);

#endif	// RENDER_VECTOR_H
