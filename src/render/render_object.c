#include "render_object.h"

#include <stddef.h>

#include <DataStuff/Heap.h>

#include "log/Logger.h"
#include "vulkan/Draw.h"
#include "vulkan/VulkanManager.h"
#include "vulkan/texture_manager.h"

#include "area_render_state.h"

typedef enum RenderObjectQuadType {
	QUAD_TYPE_MAIN,
	QUAD_TYPE_DEBUG
} RenderObjectQuadType;

/*typedef struct RenderObjectQuad {
	
	// The handle to the quad in the Vulkan model manager.
	int modelHandle;
	
	// The type of quad that this quad is.
	RenderObjectQuadType type;
	
} RenderObjectQuad;

typedef struct RenderObject {
	
	// Array of quads controlled by this render object.
	int quadCount;
	RenderObjectQuad *pQuads;
	
} RenderObject;

static RenderObject renderObjects[NUM_RENDER_OBJECT_SLOTS];*/

const int renderHandleInvalid = -1;

static Heap inactiveRenderHandles = { };

static int renderObjectQuadHandles[NUM_RENDER_OBJECT_SLOTS][MAX_NUM_RENDER_OBJECT_QUADS];
static RenderObjectQuadType renderObjectQuadTypes[NUM_RENDER_OBJECT_SLOTS][MAX_NUM_RENDER_OBJECT_QUADS];

static ModelPool getModelPoolType(const RenderObjectQuadType quadType);

bool initRenderObjectManager(void) {
	inactiveRenderHandles = newHeap(numRenderObjectSlots, sizeof(int), cmpFuncInt);
	if (!heapValidate(inactiveRenderHandles)) {
		deleteHeap(&inactiveRenderHandles);
		return false;
	}
	for (int i = 0; i < (int)numRenderObjectSlots; ++i) {
		heapPush(&inactiveRenderHandles, &i);
	}
	return true;
}

bool terminateRenderObjectManager(void) {
	deleteHeap(&inactiveRenderHandles);
	return true;
}

int loadRenderObject(const String textureID, const BoxF quadDimensions, const int numQuads, const Vector3D quadPositions[static const numQuads]) {
	if (numQuads <= 0 || numQuads > maxNumRenderObjectQuads) {
		return renderHandleInvalid;
	}
	
	int renderHandle = renderHandleInvalid;
	heapPop(&inactiveRenderHandles, &renderHandle);
	if (!validateRenderObjectHandle(renderHandle)) {
		return renderHandleInvalid;
	}
	
	for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		renderObjectQuadHandles[renderHandle][i] = -1;
	}
	
	for (int i = 0; i < numQuads; ++i) {
		const Vector4F quadPosition = {
			.x = (float)quadPositions[i].x,
			.y = (float)quadPositions[i].y,
			.z = (float)quadPositions[i].z,
			.w = 1.0F
		};
		
		int quadHandle = -1;
		const ModelLoadInfo modelLoadInfo = {
			.modelPool = modelPoolMain,
			.position = quadPosition,
			.dimensions = quadDimensions,
			.textureID = textureID,
			.color = (Vector4F){ 1.0F, 1.0F, 1.0F, 1.0F }
		};
		loadModel(modelLoadInfo, &quadHandle);
		renderObjectQuadHandles[renderHandle][i] = quadHandle;
		renderObjectQuadTypes[renderHandle][i] = QUAD_TYPE_MAIN;
	}
	
	return renderHandle;
}

void unloadRenderObject(int *const pRenderHandle) {
	if (!pRenderHandle) {
		return;
	} else if (!validateRenderObjectHandle(*pRenderHandle)) {
		return;
	}
	
	for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		if (renderObjectQuadHandles[*pRenderHandle][i] >= 0) {
			const ModelPool modelPool = getModelPoolType(renderObjectQuadTypes[*pRenderHandle][i]);
			unloadModel(modelPool, &renderObjectQuadHandles[*pRenderHandle][i]);
		}
	}
	
	heapPush(&inactiveRenderHandles, pRenderHandle);
	*pRenderHandle = renderHandleInvalid;
}

bool validateRenderObjectHandle(const int renderHandle) {
	return renderHandle >= 0 && renderHandle < (int)numRenderObjectSlots;
}

int renderObjectLoadQuad(const int renderHandle, const Vector3D position, const BoxF dimensions, const String textureID) {
	
	return -1;
}

int renderObjectLoadDebugQuad(const int renderHandle, const Vector3D position, const BoxF dimensions, const Vector4F color) {
	if (!validateRenderObjectHandle(renderHandle)) {
		return -1;
	}
	
	int quadIndex = -1;
	for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		if (renderObjectQuadHandles[renderHandle][i] < 0) {
			quadIndex = i;
			break;
		}
	}
	if (quadIndex < 0) {
		return quadIndex;
	}
	
	const Vector4F quadPosition = {
		.x = (float)position.x,
		.y = (float)position.y,
		.z = (float)position.z,
		.w = 1.0F
	};
	
	int quadHandle = -1;
	const ModelLoadInfo modelLoadInfo = {
		.modelPool = modelPoolDebug,
		.position = quadPosition,
		.dimensions = dimensions,
		.color = (Vector4F){ 1.0F, 0.0F, 0.0F, 1.0F }
	};
	loadModel(modelLoadInfo, &quadHandle);
	if (quadHandle < 0) {
		return quadIndex;
	}
	
	renderObjectQuadHandles[renderHandle][quadIndex] = quadHandle;
	renderObjectQuadTypes[renderHandle][quadIndex] = QUAD_TYPE_DEBUG;
	return quadIndex;
}

bool validateRenderObjectQuadIndex(const int quadIndex) {
	return quadIndex >= 0 && quadIndex < maxNumRenderObjectQuads;
}

bool renderObjectSetPosition(const int renderHandle, const int quadIndex, const Vector3D position) {
	if (!validateRenderObjectHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return false;
	}
	const Vector4F translation = {
		.x = (float)position.x,
		.y = (float)position.y,
		.z = (float)position.z,
		.w = 1.0F
	};
	
	const ModelPool modelPool = getModelPoolType(renderObjectQuadTypes[renderHandle][quadIndex]);
	modelSetTranslation(modelPool, renderObjectQuadHandles[renderHandle][quadIndex], translation);
	return true;
}

int renderObjectGetTextureHandle(const int renderHandle, const int quadIndex) {
	if (!validateRenderObjectHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return -1;
	}
	
	const int quadHandle = renderObjectQuadHandles[renderHandle][quadIndex];
	const ModelPool modelPool = getModelPoolType(renderObjectQuadTypes[renderHandle][quadIndex]);
	TextureState *const pTextureState = modelGetTextureState(modelPool, quadHandle);
	if (pTextureState) {
		return pTextureState->textureHandle;
	}
	return -1;
}

bool renderObjectAnimate(const int renderHandle) {
	if (!validateRenderObjectHandle(renderHandle)) {
		return false;
	}
	
	for (int quadIndex = 0; quadIndex < maxNumRenderObjectQuads; ++quadIndex) {
		const int quadHandle = renderObjectQuadHandles[renderHandle][quadIndex];
		if (quadHandle < 0) {
			continue;
		}
		
		TextureState *const pTextureState = modelGetTextureState(modelPoolMain, quadHandle);
		if (textureStateAnimate(pTextureState) == 2) {
			const unsigned int imageIndex = pTextureState->startCell + pTextureState->currentFrame;
			const ModelPool modelPool = getModelPoolType(renderObjectQuadTypes[renderHandle][quadIndex]);
			updateDrawInfo(modelPool, quadHandle, imageIndex);
		}
	}
	
	return true;
}

unsigned int renderObjectGetAnimation(const int renderHandle, const int quadIndex) {
	if (!validateRenderObjectHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return 0;
	}
	
	const int quadHandle = renderObjectQuadHandles[renderHandle][quadIndex];
	TextureState *const pTextureState = modelGetTextureState(modelPoolMain, quadHandle);
	if (pTextureState) {
		return pTextureState->currentAnimation;
	}
	return 0;
}

bool renderObjectSetAnimation(const int renderHandle, const int quadIndex, const unsigned int nextAnimation) {
	if (!validateRenderObjectHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return false;
	}
	
	const int quadHandle = renderObjectQuadHandles[renderHandle][quadIndex];
	TextureState *const pTextureState = modelGetTextureState(modelPoolMain, quadHandle);
	if (pTextureState) {
		if (!textureStateSetAnimation(pTextureState, nextAnimation)) {
			return false;
		}
		const unsigned int imageIndex = pTextureState->startCell + pTextureState->currentFrame;
		updateDrawInfo(modelPoolMain, quadHandle, imageIndex);
		return true;
	}
	return false;
}

static ModelPool getModelPoolType(const RenderObjectQuadType quadType) {
	switch (quadType) {
		default:
		case QUAD_TYPE_MAIN: return modelPoolMain;
		case QUAD_TYPE_DEBUG: return modelPoolDebug;
	};
}


