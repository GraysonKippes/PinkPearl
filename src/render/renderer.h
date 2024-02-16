#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

#include "game/area/room.h"
#include "util/extent.h"

#include "model.h"
#include "projection.h"
#include "vulkan/texture.h"

extern const projection_bounds_t projection_bounds_8x5;

void init_renderer(void);
void terminate_renderer(void);

void render_frame(float tick_delta_time);

void settle_render_positions(void);
void update_projection_bounds(projection_bounds_t projection_bounds);
void set_tilemap(const char *const tilemap_name);

void upload_model(uint32_t slot, model_t model, const char *const texture_name);
void upload_room_model(const room_t room);

void begin_room_scroll(const extent_t room_extent, const offset_t previous_room_position, const offset_t next_room_position);
bool is_camera_scrolling(void);

#endif	// RENDERER_H
