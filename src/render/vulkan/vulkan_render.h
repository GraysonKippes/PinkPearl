#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "game/area/room.h"
#include "render/color.h"
#include "render/model.h"
#include "render/projection.h"
#include "render/render_object.h"
#include "render/texture_state.h"
#include "render/math/vector3F.h"

#include "texture.h"

// This file holds all functionality for drawing the scene with the Vulkan API.

void create_vulkan_render_objects(void);

void destroy_vulkan_render_objects(void);

void set_clear_color(color3F_t color);

void stage_model_data(uint32_t slot, model_t model);

void draw_frame(const float tick_delta_time, const vector3F_t camera_position, const projection_bounds_t projection_bounds);

#endif	// VULKAN_RENDER_H
