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

void createVulkanRenderObjects(void);
void destroyVulkanRenderObjects(void);

void initTextureDescriptors(void);

int loadQuad(const DimensionsF quadDimensions, const Vector4F quadPosition, const TextureState quadTextureState);
void unloadQuad(const int quadID);
bool validateQuadID(const int quadID);

// Quad transform mutators.
bool setQuadTranslation(const int quadID, const Vector4F translation);
bool setQuadScaling(const int quadID, const Vector4F scaling);
bool setQuadRotation(const int quadID, const Vector4F rotation);

TextureState *getQuadTextureState(const int quadID);

void updateDrawData(const int quadID, const unsigned int imageIndex);

void drawFrame(const float deltaTime, const Vector4F cameraPosition, const projection_bounds_t projectionBounds);

#endif	// VULKAN_RENDER_H
