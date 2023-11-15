#ifndef RENDERER_H
#define RENDERER_H

#include "game/area/room.h"

#include "projection.h"

extern const projection_bounds_t projection_bounds_8x5;

void init_renderer(void);

void terminate_renderer(void);

void render_frame(double tick_delta);

void update_projection_bounds(projection_bounds_t projection_bounds);

void upload_room_model(room_t room);

#endif	// RENDERER_H
