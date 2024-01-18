#ifndef RENDER_OBJECT_H
#define RENDER_OBJECT_H

#include <stdbool.h>
#include <stdint.h>

#include "math/vector3F.h"

#include "render_config.h"
#include "render_position.h"

typedef uint32_t render_handle_t;

extern const uint32_t render_handle_invalid;

// The render object is an abstract concept, and is separated into various components which are
// stored in their own arrays, for easy vectorization by the renderer.
extern render_position_t render_object_positions[NUM_RENDER_OBJECT_SLOTS];

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

#endif	// RENDER_OBJECT_H
