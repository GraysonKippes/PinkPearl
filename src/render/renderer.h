#ifndef RENDERER_H
#define RENDERER_H

#include "projection.h"

extern const projection_bounds_t projection_bounds_8x5;

void render_frame(double tick_delta);

void update_projection_bounds(projection_bounds_t projection_bounds);

#endif	// RENDERER_H
