#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

#include <stdint.h>

#include "game/area/room.h"
#include "render/color.h"
#include "render/model.h"
#include "render/projection.h"
#include "render/render_object.h"

// This file holds all functionality for drawing the scene with the Vulkan API.

void create_vulkan_render_objects(void);

void destroy_vulkan_render_objects(void);

void set_clear_color(color3F_t color);

void stage_model_data(render_handle_t handle, model_t model);

// Dispatches a work load to the compute_matrices shader, computing a transformation matrix for each render object.
void compute_matrices(uint32_t num_inputs, float delta_time, render_position_t camera_position, projection_bounds_t projection_bounds, render_position_t *positions);

void create_room_texture(room_t room);

void draw_frame(double delta_time, projection_bounds_t projection_bounds);

#endif	// VULKAN_RENDER_H
