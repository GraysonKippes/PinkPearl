#ifndef COMPUTE_MATRICES_H
#define COMPUTE_MATRICES_H

#include <vulkan/vulkan.h>

#include "render/projection.h"
#include "render/render_object.h"
#include "render/math/vector3F.h"

extern VkSemaphore compute_matrices_semaphore;

extern const VkDeviceSize matrix_data_size;

bool init_compute_matrices(const VkDevice vk_device);
void terminate_compute_matrices(void);

void compute_matrices(float delta_time, projection_bounds_t projection_bounds, vector3F_t camera_position, render_position_t *positions);

#endif	// COMPUTE_MATRICES_H
