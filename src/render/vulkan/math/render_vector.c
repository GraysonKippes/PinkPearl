#include "render_vector.h"

#include <stddef.h>

void render_vector_set(RenderVector *const render_vector_ptr, const Vector4F new_vector) {
	if (render_vector_ptr == NULL) {
		return;
	}
	render_vector_ptr->previous = render_vector_ptr->current;
	render_vector_ptr->current = new_vector;
}

void render_vector_reset(RenderVector *const render_vector_ptr, const Vector4F new_vector) {
	if (render_vector_ptr == NULL) {
		return;
	}
	render_vector_ptr->current = new_vector;
	render_vector_ptr->previous = new_vector;
}

void render_vector_settle(RenderVector *const render_vector_ptr) {
	if (render_vector_ptr == NULL) {
		return;
	}
	render_vector_ptr->previous = render_vector_ptr->current;
}
