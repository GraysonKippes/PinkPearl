#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "game/area/room.h"
#include "math/Box.h"
#include "render/area_render_state.h"
#include "render/render_object.h"
#include "render/texture_state.h"

#include "texture.h"
#include "math/projection.h"

// This file holds all functionality for drawing the scene with the Vulkan API.

void createVulkanRenderObjects(void);
void destroyVulkanRenderObjects(void);

void initTextureDescriptors(void);

int loadQuad(const BoxF dimensions, const Vector4F position, const TextureState textureState);

void unloadQuad(int *const pQuadHandle);

bool validateQuadHandle(const int quadHandle);

// Quad transform mutators.
bool setQuadTranslation(const int quadHandle, const Vector4F translation);
bool setQuadScaling(const int quadHandle, const Vector4F scaling);
bool setQuadRotation(const int quadHandle, const Vector4F rotation);

TextureState *getQuadTextureState(const int quadHandle);

void updateDrawData(const int quadHandle, const unsigned int imageIndex);

void drawFrame(const float deltaTime, const Vector4F cameraPosition, const ProjectionBounds projectionBounds);

#endif	// VULKAN_RENDER_H
