#ifndef COMPUTE_AREA_MESH_H
#define COMPUTE_AREA_MESH_H

#include <vulkan/vulkan.h>

#include "game/area/area.h"

void init_compute_area_mesh(const VkDevice vk_device);
void terminate_compute_area_mesh(void);

void compute_area_mesh(const area_t area);

#endif	//COMPUTE_AREA_MESH_H
