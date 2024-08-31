#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

#include "math/Box.h"
#include "math/vector4F.h"
#include "util/string.h"

typedef struct ModelPool_T *ModelPool;

void createModelPool(const uint32_t firstIndex, const uint32_t indexCount, const int32_t vertexCount, const uint32_t maxModelCount, ModelPool *pOutModelPool); 

void deleteModelPool(ModelPool *const pModelPool);

void loadModel(ModelPool modelPool, const Vector4F position, const BoxF dimensions, const String textureID, int *const pModelHandle);

void unloadModel(ModelPool modelPool, int *const pModelHandle);

void modelSetTranslation(ModelPool modelPool, const int modelHandle, const Vector4F translation);

void modelSetScaling(ModelPool modelPool, const int modelHandle, const Vector4F scaling);

void modelSetRotation(ModelPool modelPool, const int modelHandle, const Vector4F rotation);

#endif // DRAW_H