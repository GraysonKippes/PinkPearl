#include "render_position.h"

#include <stddef.h>

void update_render_position(render_position_t *render_position_ptr, vector3F_t new_position) {

	if (render_position_ptr == NULL) {
		return;
	}

	render_position_ptr->previous_position = render_position_ptr->position;
	render_position_ptr->position = new_position;
}

void reset_render_position(render_position_t *render_position_ptr, vector3F_t new_position) {

	if (render_position_ptr == NULL) {
		return;
	}

	render_position_ptr->position = new_position;
	render_position_ptr->previous_position = new_position;
}

void settle_render_position(render_position_t *render_position_ptr) {

	if (render_position_ptr == NULL) {
		return;
	}

	render_position_ptr->previous_position = render_position_ptr->position;
}
