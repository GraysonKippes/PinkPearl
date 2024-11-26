#ifndef COMPUTE_MATRICES_H
#define COMPUTE_MATRICES_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

#include "../synchronization.h"
#include "../math/projection.h"
#include "../math/render_vector.h"

extern TimelineSemaphore computeMatricesSemaphore;

extern const VkDeviceSize matrix_data_size;

bool initComputeMatrices(const VkDevice vkDevice);
void terminateComputeMatrices(void);

void computeMatrices(const uint32_t transformBufferDescriptorHandle, const uint32_t matrixBufferDescriptorHandle, 
		const float deltaTime, const ProjectionBounds projectionBounds, const Vector4F cameraPosition, 
		const uint32_t *const pCameraFlags, const ModelTransform *const transforms);

#endif	// COMPUTE_MATRICES_H
