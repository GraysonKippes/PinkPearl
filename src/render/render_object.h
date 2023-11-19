#ifndef RENDER_OBJECT_H
#define RENDER_OBJECT_H

#include <stdint.h>

#include "math/vector3F.h"

#include "render_config.h"



typedef uint32_t render_handle_t;

render_handle_t create_render_object(void);

void destroy_render_object(render_handle_t);



// The render object is an abstract concept, and is separated into various components which are
// stored in their own arrays, for easy vectorization by the renderer.

typedef struct render_position_t {
	
	vector3F_t position;

	vector3F_t previous_position;

} render_position_t;

extern render_position_t render_object_positions[NUM_RENDER_OBJECT_SLOTS];

void update_render_position(render_position_t *render_position_ptr, vector3F_t new_position);

void reset_render_position(render_position_t *render_position_ptr, vector3F_t new_position);



#endif	// RENDER_OBJECT_H
