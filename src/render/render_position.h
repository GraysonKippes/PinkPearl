#ifndef RENDER_POSITION_H
#define RENDER_POSITION_H

#include "math/vector3F.h"

typedef struct render_position_t {
	
	vector3F_t position;

	vector3F_t previous_position;

} render_position_t;

// Updates the render position, setting the previous position to what the position was before the update.
void update_render_position(render_position_t *const render_position_ptr, vector3F_t new_position);

// Force-updates both the current position and the previous position to the new position.
void reset_render_position(render_position_t *const render_position_ptr, vector3F_t new_position);

// Sets the previous position to the current position.
void settle_render_position(render_position_t *const render_position_ptr);

#endif	// RENDER_POSITION_H
