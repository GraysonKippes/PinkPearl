#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

#include <stdint.h>

#include "game/area/room.h"
#include "render/color.h"
#include "render/model.h"
#include "render/projection.h"
#include "render/render_object.h"

#include "texture.h"

// This file holds all functionality for drawing the scene with the Vulkan API.

void create_vulkan_render_objects(void);

void destroy_vulkan_render_objects(void);

void enable_render_object_slot(uint32_t slot);

void disable_render_object_slot(uint32_t slot);

void set_clear_color(color3F_t color);

void stage_model_data(uint32_t slot, model_t model);

texture_t *get_model_texture_ptr(uint32_t slot);

void set_model_texture(uint32_t slot, texture_t texture);

void create_room_texture(room_t room);

void draw_frame(double delta_time, projection_bounds_t projection_bounds);

#endif	// VULKAN_RENDER_H
