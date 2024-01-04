#ifndef RENDER_OBJECT_H
#define RENDER_OBJECT_H

#include <stdbool.h>
#include <stdint.h>

#include "math/vector3F.h"

#include "render_config.h"



typedef uint32_t render_handle_t;

extern const uint32_t render_handle_invalid;

render_handle_t load_render_object(void);

void unload_render_object(render_handle_t *handle_ptr);

// Returns true if the render handle is both below the number of render object slots 
// 	and below the bitwidth of the render object slot flags;
// 	returns false otherwise.
bool validate_render_handle(render_handle_t handle);

// Returns true if the render handle goes to a render object slot dedicated to room render objects.
bool is_render_handle_to_room_render_object(render_handle_t handle);

render_handle_t create_render_object(void);

void destroy_render_object(render_handle_t);



// The render object is an abstract concept, and is separated into various components which are
// stored in their own arrays, for easy vectorization by the renderer.

typedef struct render_position_t {
	
	vector3F_t position;

	vector3F_t previous_position;

} render_position_t;

extern render_position_t render_object_positions[NUM_RENDER_OBJECT_SLOTS];

// Updates the render position, setting the previous position to what the position was before the update.
void update_render_position(render_position_t *render_position_ptr, vector3F_t new_position);

// Force-updates both the current position and the previous position to the new position.
void reset_render_position(render_position_t *render_position_ptr, vector3F_t new_position);



#endif	// RENDER_OBJECT_H
