#include "renderer.h"

#include "vulkan/vulkan_manager.h"

const projection_bounds_t projection_bounds_8x5 = {
	.m_left = -4.0F, 	.m_right = 4.0F,
	.m_bottom = -2.5F, 	.m_top = 2.5F,
	.m_near = 15.0F,	.m_far = -15.0F

};

static projection_bounds_t current_projection_bounds;

void render_frame(double tick_delta) {
	draw_frame(tick_delta, current_projection_bounds);
}

void update_projection_bounds(projection_bounds_t new_projection_bounds) {
	current_projection_bounds = new_projection_bounds;
}
