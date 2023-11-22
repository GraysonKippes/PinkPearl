#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

#include "game/area/room.h"

#include "model.h"
#include "projection.h"
#include "vulkan/texture.h"

extern const projection_bounds_t projection_bounds_8x5;

void init_renderer(void);

void terminate_renderer(void);

void render_frame(float tick_delta_time);

void update_projection_bounds(projection_bounds_t projection_bounds);

void upload_model(uint32_t slot, model_t model, texture_t texture);

void upload_room_model(room_t room);

#endif	// RENDERER_H
