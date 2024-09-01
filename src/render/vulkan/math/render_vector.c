#include "render_vector.h"

#include <stddef.h>

void renderVectorSet(RenderVector *const pRenderVector, const Vector4F newVector) {
	if (pRenderVector == nullptr) {
		return;
	}
	pRenderVector->previous = pRenderVector->current;
	pRenderVector->current = newVector;
}

void renderVectorReset(RenderVector *const pRenderVector, const Vector4F newVector) {
	if (pRenderVector == nullptr) {
		return;
	}
	pRenderVector->current = newVector;
	pRenderVector->previous = newVector;
}

void renderVectorSettle(RenderVector *const pRenderVector) {
	if (pRenderVector == nullptr) {
		return;
	}
	pRenderVector->previous = pRenderVector->current;
}

ModelTransform makeModelTransform(const Vector4F translation, const Vector4F scaling, const Vector4F rotation) {
	return (ModelTransform){
		.translation = (RenderVector){ .current = translation, .previous = translation },
		.scaling = (RenderVector){ .current = scaling, .previous = scaling },
		.rotation = (RenderVector){ .current = rotation, .previous = rotation }
	};
}