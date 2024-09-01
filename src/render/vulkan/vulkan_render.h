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

void createVulkanRenderObjects(void);

void destroyVulkanRenderObjects(void);

void initTextureDescriptors(void);

[[deprecated]]
int loadQuad(const BoxF dimensions, const Vector4F position, const TextureState textureState);

[[deprecated]]
void unloadQuad(int *const pQuadHandle);

[[deprecated]]
bool validateQuadHandle(const int quadHandle);

[[deprecated]]
bool setQuadTranslation(const int quadHandle, const Vector4F translation);

[[deprecated]]
bool setQuadScaling(const int quadHandle, const Vector4F scaling);

[[deprecated]]
bool setQuadRotation(const int quadHandle, const Vector4F rotation);

[[deprecated]]
TextureState *getQuadTextureState(const int quadHandle);

[[deprecated]]
void updateDrawData(const int quadHandle, const unsigned int imageIndex);

void drawFrame(const float deltaTime, const Vector4F cameraPosition, const ProjectionBounds projectionBounds);

#endif	// VULKAN_RENDER_H
