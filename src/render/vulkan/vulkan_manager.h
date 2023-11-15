#ifndef VULKAN_MANAGER_H
#define VULKAN_MANAGER_H

#include "game/area/room.h"
#include "render/color.h"
#include "render/model.h"
#include "render/projection.h"
#include "render/render_object.h"

// This file contains all the vulkan objects and functions to create them and destroy them.

// Creates all Vulkan objects needed for the rendering system.
void create_vulkan_objects(void);

// Destroys the Vulkan objects created for the rendering system.
void destroy_vulkan_objects(void);

void stage_model_data(render_handle_t handle, model_t model);

// Dispatches a work load to the compute_matrices shader, computing a transformation matrix for each render object.
void compute_matrices(uint32_t num_inputs, float delta_time, render_position_t camera_position, projection_bounds_t projection_bounds, render_position_t *positions);

void create_room_texture(room_t room);

void draw_frame(double delta_time, projection_bounds_t projection_bounds);

#endif	// VULKAN_MANAGER_H
