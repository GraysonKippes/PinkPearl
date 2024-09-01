#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "game/area/room.h"
#include "math/Box.h"
#include "render/area_render_state.h"
#include "render/render_object.h"

#include "texture.h"
#include "TextureState.h"
#include "math/projection.h"

// This file holds all functionality for drawing the scene with the Vulkan API.

void initTextureDescriptors(void);

void drawFrame(const float deltaTime, const Vector4F cameraPosition, const ProjectionBounds projectionBounds);

#endif	// VULKAN_RENDER_H
