#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

#include "math/Box.h"
#include "math/vector4F.h"
#include "util/string.h"

#include "Buffer2.h"
#include "TextureState.h"
#include "math/render_vector.h"

typedef struct ModelPool_T *ModelPool;

extern const uint32_t drawCountSize;

extern const uint32_t drawCommandStride;

void createModelPool(const Buffer buffer, const int32_t bufferSubrangeIndex, 
		const uint32_t firstVertex, const uint32_t vertexCount, 
		const uint32_t firstIndex, const uint32_t indexCount, 
		const uint32_t firstDescriptorIndex, const uint32_t maxModelCount, 
		ModelPool *pOutModelPool);

void deleteModelPool(ModelPool *const pModelPool);

uint32_t modelPoolGetMaxModelCount(const ModelPool modelPool);

VkDescriptorBufferInfo modelPoolGetBufferDescriptorInfo(const ModelPool modelPool);

void modelPoolGetDrawCommandArguments(const ModelPool modelPool, uint32_t *const pMaxDrawCount, uint32_t *const pStride);

void loadModel(ModelPool modelPool, const Vector4F position, const BoxF dimensions, const String textureID, int *const pModelHandle);

void unloadModel(ModelPool modelPool, int *const pModelHandle);

void modelSetTranslation(ModelPool modelPool, const int modelHandle, const Vector4F translation);

void modelSetScaling(ModelPool modelPool, const int modelHandle, const Vector4F scaling);

void modelSetRotation(ModelPool modelPool, const int modelHandle, const Vector4F rotation);

// TODO - replace these two functions with specific control functions
TextureState *modelGetTextureState(ModelPool modelPool, const int modelHandle);
void updateDrawInfo(ModelPool modelPool, const int modelHandle, const unsigned int imageIndex);
ModelTransform *getModelTransforms(const ModelPool modelPool);

#endif // DRAW_H