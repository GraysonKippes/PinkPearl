#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "game/area/room.h"
#include "render/color.h"
#include "render/model.h"
#include "render/projection.h"
#include "render/render_object.h"
#include "render/math/vector3F.h"

#include "texture.h"

// This file holds all functionality for drawing the scene with the Vulkan API.

void create_vulkan_render_objects(void);

void destroy_vulkan_render_objects(void);

bool is_render_object_slot_enabled(uint32_t slot);

void enable_render_object_slot(uint32_t slot);

void disable_render_object_slot(uint32_t slot);

void set_clear_color(color3F_t color);

void stage_model_data(uint32_t slot, model_t model);

texture_t *get_model_texture_ptr(uint32_t slot);

void set_model_texture(uint32_t slot, texture_t texture);

void create_room_texture(room_t room, uint32_t render_object_slot);

void draw_frame(const float tick_delta_time, const vector3F_t camera_position, const projection_bounds_t projection_bounds);

#endif	// VULKAN_RENDER_H
