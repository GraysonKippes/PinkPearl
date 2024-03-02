#ifndef RENDER_OBJECT_H
#define RENDER_OBJECT_H

#include <stdbool.h>
#include <stdint.h>

#include "math/vector3F.h"

#include "render_config.h"
#include "render_position.h"
#include "texture_state.h"

typedef uint32_t render_handle_t;

extern const uint32_t render_handle_invalid;

// The render object is an abstract concept, and is separated into various components which are
// stored in their own arrays, for easy vectorization by the renderer.
extern render_position_t render_object_positions[NUM_RENDER_OBJECT_SLOTS];
extern texture_state_t render_object_texture_states[NUM_RENDER_OBJECT_SLOTS];

render_handle_t load_render_object(void);
void unload_render_object(render_handle_t *handle_ptr);

bool is_render_object_slot_enabled(const uint32_t slot);
void enable_render_object_slot(const uint32_t slot);

// Returns true if the render handle is both below the number of render object slots 
// 	and below the bitwidth of the render object slot flags;
// 	returns false otherwise.
bool validate_render_handle(render_handle_t handle);

// Destroys the previous texture state of the render object controlled by the handle, and sets it to the next texture state.
bool swap_render_object_texture_state(const render_handle_t render_handle, const texture_state_t texture_state);

#endif	// RENDER_OBJECT_H
