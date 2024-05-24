#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "game/area/room.h"
#include "math/dimensions.h"
#include "render/area_render_state.h"
#include "render/render_object.h"
#include "render/texture_state.h"

#include "texture.h"
#include "math/projection.h"

// This file holds all functionality for drawing the scene with the Vulkan API.

int loadQuad(const DimensionsF quadDimensions);
void unloadQuad(const int quadID);
bool validateQuadID(const int quadID);

void create_vulkan_render_objects(void);
void destroy_vulkan_render_objects(void);
void drawFrame(const float deltaTime, const vector4F_t cameraPosition, const projection_bounds_t projectionBounds, const RenderTransform *const renderObjTransforms);

#endif	// VULKAN_RENDER_H
