#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

#include "math/Box.h"
#include "math/vector4F.h"
#include "util/string.h"

#include "Buffer2.h"
#include "GraphicsPipeline.h"
#include "TextureState.h"
#include "math/render_vector.h"



typedef struct ModelPool_T *ModelPool;

extern const uint32_t drawCountSize;

extern const uint32_t drawCommandStride;

typedef struct ModelPoolCreateInfo {
	
	Buffer buffer;
	int32_t bufferSubrangeIndex;
	
	GraphicsPipeline graphicsPipeline;
	
	uint32_t firstVertex;
	uint32_t vertexCount;
	
	uint32_t firstIndex;
	uint32_t indexCount;
	
	uint32_t firstDescriptorIndex;
	
	uint32_t maxModelCount;
	
} ModelPoolCreateInfo;

void createModelPool(const Buffer buffer, const int32_t bufferSubrangeIndex, 
		const uint32_t firstVertex, const uint32_t vertexCount, 
		const uint32_t firstIndex, const uint32_t indexCount, 
		const uint32_t firstDescriptorIndex, const uint32_t maxModelCount, 
		ModelPool *pOutModelPool);

void deleteModelPool(ModelPool *const pModelPool);

uint32_t modelPoolGetMaxModelCount(const ModelPool modelPool);

VkDescriptorBufferInfo modelPoolGetBufferDescriptorInfo(const ModelPool modelPool);

void modelPoolGetDrawCommandArguments(const ModelPool modelPool, uint32_t *const pMaxDrawCount, uint32_t *const pStride);

VkDeviceSize modelPoolGetDrawInfoBufferOffset(const ModelPool modelPool);



typedef struct ModelCreateInfo {
	
	// Necessary parameters.
	
	// The model pool into which to load the model.
	ModelPool modelPool;
	
	// The initial position of the model.
	Vector4F position;
	
	// The quadrangle-vertices of the model.
	BoxF dimensions;
	
	// Optional parameters, depending on which resources the model needs.
	
	// The color of the model if there is a color vertex attribute.
	Vector4F color;
	
	// The ID of the texture to load for this model, if the model uses a texture.
	String textureID;
	
} ModelCreateInfo;

void loadModel(ModelPool modelPool, const Vector4F position, const BoxF dimensions, const String textureID, int *const pModelHandle);

void loadModel2(const ModelCreateInfo, int *const pModelHandle);

void unloadModel(ModelPool modelPool, int *const pModelHandle);

void modelSetTranslation(ModelPool modelPool, const int modelHandle, const Vector4F translation);

void modelSetScaling(ModelPool modelPool, const int modelHandle, const Vector4F scaling);

void modelSetRotation(ModelPool modelPool, const int modelHandle, const Vector4F rotation);



// TODO - replace these two functions with specific control functions
TextureState *modelGetTextureState(ModelPool modelPool, const int modelHandle);
void updateDrawInfo(ModelPool modelPool, const int modelHandle, const unsigned int imageIndex);
ModelTransform *getModelTransforms(const ModelPool modelPool);

#endif // DRAW_H