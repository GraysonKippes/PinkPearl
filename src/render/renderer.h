#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

#include "game/area/room.h"
#include "util/extent.h"

#include "model.h"

void init_renderer(void);
void terminate_renderer(void);

void render_frame(float tick_delta_time);
void settle_render_positions(void);
void upload_model(uint32_t slot, model_t model, const char *const texture_name);

#endif	// RENDERER_H
