#ifndef COMPUTE_MATRICES_H
#define COMPUTE_MATRICES_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

#include "../math/projection.h"
#include "../math/render_vector.h"

extern VkSemaphore compute_matrices_semaphore;

extern const VkDeviceSize matrix_data_size;

bool init_compute_matrices(const VkDevice vk_device);
void terminate_compute_matrices(void);

void computeMatrices(const float deltaTime, const projection_bounds_t projectionBounds, const vector4F_t cameraPosition, const RenderTransform *const transforms);

#endif	// COMPUTE_MATRICES_H
