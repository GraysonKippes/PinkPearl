#ifndef COMPUTE_MATRICES_H
#define COMPUTE_MATRICES_H

#include <vulkan/vulkan.h>

#include "render/projection.h"
#include "render/render_object.h"

extern const VkDeviceSize matrix_data_size;

void init_compute_matrices(void);

void terminate_compute_matrices(void);

void compute_matrices(float delta_time, projection_bounds_t projection_bounds, render_position_t camera_position, render_position_t *positions);

#endif	// COMPUTE_MATRICES_H
