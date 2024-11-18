#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

#include "math/Box.h"
#include "math/Vector.h"
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
		
void createModelPool(const ModelPoolCreateInfo createInfo, ModelPool *const pOutModelPool);

void deleteModelPool(ModelPool *const pModelPool);

uint32_t modelPoolGetMaxModelCount(const ModelPool modelPool);

VkDescriptorBufferInfo modelPoolGetBufferDescriptorInfo(const ModelPool modelPool);

void modelPoolGetDrawCommandArguments(const ModelPool modelPool, uint32_t *const pMaxDrawCount, uint32_t *const pStride);

VkDeviceSize modelPoolGetDrawInfoBufferOffset(const ModelPool modelPool);

uint32_t modelPoolGetDrawInfoBufferHandle(const ModelPool modelPool);



typedef struct ModelLoadInfo {
	
	// Necessary parameters.
	
	// The model pool into which to load the model.
	ModelPool modelPool;
	
	// The initial position of the model.
	Vector4F position;
	
	// The quadrangle-vertices of the model.
	BoxF dimensions;
	
	// Optional parameters, depending on which resources the model needs.
	
	// The ID of the texture to load for this model, if the model uses a texture.
	String textureID;
	
	// The color of the model if there is a color vertex attribute.
	Vector4F color;
	
	uint32_t imageIndex;
	
} ModelLoadInfo;

void loadModel(const ModelLoadInfo loadInfo, int *const pModelHandle);

void unloadModel(ModelPool modelPool, int *const pModelHandle);

void modelSetTranslation(ModelPool modelPool, const int modelHandle, const Vector4F translation);

void modelSetScaling(ModelPool modelPool, const int modelHandle, const Vector4F scaling);

void modelSetRotation(ModelPool modelPool, const int modelHandle, const Vector4F rotation);



// TODO - replace these functions with specific control functions
TextureState *modelGetTextureState(ModelPool modelPool, const int modelHandle);
void updateDrawInfo(ModelPool modelPool, const int modelHandle, const unsigned int imageIndex);
ModelTransform *getModelTransforms(const ModelPool modelPool);

#endif // DRAW_H