#include "Draw.h"

#include <stdlib.h>

#include "frame.h"
#include "TextureState.h"
#include "vertex_input.h"
#include "math/render_vector.h"

typedef struct DrawInfo {
	
	/* Indirect draw command parameters */
	
	uint32_t indexCount;
	uint32_t instanceCount;	// IGNORED
	uint32_t firstIndex;
	int32_t vertexOffset;
	uint32_t firstInstance;	// IGNORED
	
	/* Additional draw parameters */
	
	// Indexes into global descriptor arrays.
	int32_t modelID;
	
	// Indexes into a texture array.
	uint32_t imageIndex;
	
} DrawInfo;

static const uint32_t indirectDrawStride = sizeof(DrawInfo);

// Holds data for a batch of models to be drawn with a single indirect draw call.
// Controls how the parameters for the indirect draw call are generated.
struct ModelPool_T {
	
	// In the current system, all models in the same pool use the same structure, aka, the same indices with the same number of vertices.
	// As a result, the positions of the first vertex and first index for a particular model pool in the vertex/index buffer(s) is constant
	// 	and the position of a particular mesh can be calculated by multiplying the mesh size by its relative position in the mesh array.
	
	// Each pool has its own sub-array of vertices within the vertex buffer(s); this is the position of the first element in that sub-array.
	uint32_t firstVertex;
	
	// Number of vertices of each model in this pool, e.g. four for quads/rects.
	uint32_t vertexCount;
	
	// Each pool has its own sub-array of indices within the index buffer(s); this is the position of the first element in that sub-array.
	uint32_t firstIndex;
	
	// Number of indices of each model in this pool.
	uint32_t indexCount;
	
	// The maximum number of models.
	uint32_t maxModelCount;
	
	/* MODEL OBJECT FIELDS */
	
	// Indicates whether a model is loaded into a particular model array slot.
	bool *pSlotFlags;
	
	// The index into the array of draw info structures uploaded to the GPU.
	// Used to update a particular model's draw parameters without rebuilding the whole array of draw info structures.
	uint32_t *pDrawInfoIndices;
	
	ModelTransform *pModelTransforms;
	
	TextureState *pTextureStates;
	
	/* DRAW INFO */
	
	// The number of draw infos, but not the space allocated.
	// Equal to the number of *active* models.
	uint32_t drawInfoCount;
	
	// All of the draw parameters for each of the models.
	// DrawInfoIndex indexes into this array.
	DrawInfo *pDrawInfos;
};

void createModelPool(const uint32_t firstIndex, const uint32_t indexCount, const int32_t vertexCount, const uint32_t maxModelCount, ModelPool *pOutModelPool) {
	
	ModelPool modelPool = malloc(sizeof(struct ModelPool_T));
	if (!modelPool) {
		return;
	}
	
	modelPool->firstIndex = firstIndex;
	modelPool->indexCount = indexCount;
	modelPool->vertexCount = vertexCount;
	modelPool->maxModelCount = maxModelCount;
	
	modelPool->pSlotFlags = calloc(maxModelCount, sizeof(bool));
	if (!modelPool->pSlotFlags) {
		free(modelPool);
		return;
	}
	
	modelPool->pDrawInfoIndices = calloc(maxModelCount, sizeof(uint32_t));
	if (!modelPool->pDrawInfoIndices) {
		free(modelPool->pSlotFlags);
		free(modelPool);
		return;
	}
	
	modelPool->pModelTransforms = calloc(maxModelCount, sizeof(ModelTransform));
	if (!modelPool->pModelTransforms) {
		free(modelPool->pSlotFlags);
		free(modelPool->pDrawInfoIndices);
		free(modelPool);
		return;
	}
	
	modelPool->pTextureStates = calloc(maxModelCount, sizeof(TextureState));
	if (!modelPool->pTextureStates) {
		free(modelPool->pSlotFlags);
		free(modelPool->pDrawInfoIndices);
		free(modelPool->pModelTransforms);
		free(modelPool);
		return;
	}
	
	modelPool->pDrawInfos = calloc(maxModelCount, sizeof(DrawInfo));
	if (!modelPool->pTextureStates) {
		free(modelPool->pSlotFlags);
		free(modelPool->pDrawInfoIndices);
		free(modelPool->pModelTransforms);
		free(modelPool->pTextureStates);
		free(modelPool);
		return;
	}
	
	*pOutModelPool = modelPool;
}

void deleteModelPool(ModelPool *const pModelPool) {
	free((*pModelPool)->pSlotFlags);
	free((*pModelPool)->pDrawInfoIndices);
	free((*pModelPool)->pModelTransforms);
	free((*pModelPool)->pTextureStates);
	free((*pModelPool)->pDrawInfos);
	free((*pModelPool));
	*pModelPool = nullptr;
}

void loadModel(ModelPool modelPool, const Vector4F position, const BoxF dimensions, const String textureID, int *const pModelHandle) {
	
	// For simplicity's sake, just use a linear search to find the first available slot.
	// TODO - maybe optimize this with a data structure?
	uint32_t modelIndex = 0;
	for (; modelIndex < modelPool->maxModelCount; ++modelIndex) {
		if (modelPool->pSlotFlags[modelIndex]) {
			break;
		}
	}
	
	if (modelIndex >= modelPool->maxModelCount) {
		*pModelHandle = -1;
		return;
	}
	
	// Reset model transform
	// Load texture state
	
	ModelTransform modelTransform = makeModelTransform(position, zeroVector4F, zeroVector4F);
	
	TextureState textureState = newTextureState(textureID);
	
	// Generate mesh
	// Upload mesh to vertex buffer(s)
	
	const float vertices[NUM_VERTICES_PER_QUAD * VERTEX_INPUT_ELEMENT_STRIDE] = {
		// Positions							Texture			Color
		dimensions.x1, dimensions.y1, 0.0F,		0.0F, 1.0F,		1.0F, 1.0F, 1.0F,	// 0: Bottom-left
		dimensions.x1, dimensions.y2, 0.0F,		0.0F, 0.0F,		1.0F, 1.0F, 1.0F,	// 1: Top-left
		dimensions.x2, dimensions.y2, 0.0F,		1.0F, 0.0F,		1.0F, 1.0F, 1.0F,	// 2: Top-right
		dimensions.x2, dimensions.y1, 0.0F,		1.0F, 1.0F,		1.0F, 1.0F, 1.0F	// 3: Bottom-right
	};
	
	
	
	// Generate draw parameters
	// Insert draw info into appropriate place into draw info array, depending on depth
	// Upload new draw info array to buffer
	
	const DrawInfo drawInfo = {
		.indexCount = modelPool->indexCount,
		.instanceCount = 1,
		.firstIndex = modelPool->firstIndex,
		.vertexOffset = modelPool->firstVertex + modelPool->vertexCount * modelIndex,
		.firstInstance = 0,
		.modelID = modelIndex,
		.imageIndex = 0
	};
	
	
	
	*pModelHandle = (int)modelIndex;
}

void unloadModel(ModelPool modelPool, int *const pModelHandle) {
	
	uint32_t modelIndex = (uint32_t)*pModelHandle;
	modelPool->pSlotFlags[modelIndex] = false;
	
	// Remove model's draw info from model pool's array of draw infos
	// If any other draw infos are moved around, adjust their models' draw info indices
	
	*pModelHandle = -1;
}

void modelSetTranslation(ModelPool modelPool, const int modelHandle, const Vector4F translation) {
	
}

void modelSetScaling(ModelPool modelPool, const int modelHandle, const Vector4F scaling) {
	
}

void modelSetRotation(ModelPool modelPool, const int modelHandle, const Vector4F rotation) {
	
}