#ifndef RENDERER_H
#define RENDERER_H

#include "area_render_state.h"

extern AreaRenderState globalAreaRenderState;

void init_renderer(void);
void terminate_renderer(void);

void render_frame(float tick_delta_time);

#endif	// RENDERER_H
