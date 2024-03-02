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

void compute_matrices(const float delta_time, const projection_bounds_t projection_bounds, const vector3F_t camera_position, const render_position_t *const positions);

#endif	// COMPUTE_MATRICES_H
