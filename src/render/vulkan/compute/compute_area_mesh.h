#ifndef COMPUTE_AREA_MESH_H
#define COMPUTE_AREA_MESH_H

#include <vulkan/vulkan.h>

#include "game/area/area.h"

[[deprecated("area mesh compute is to be removed.")]]
void init_compute_area_mesh(const VkDevice vk_device);

[[deprecated("area mesh compute is to be removed.")]]
void terminate_compute_area_mesh(void);

[[deprecated("area mesh compute is to be removed.")]]
void compute_area_mesh(const area_t area);

#endif	// COMPUTE_AREA_MESH_H
