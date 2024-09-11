#include "render_object.h"

#include <stddef.h>

#include <DataStuff/Heap.h>

#include "log/Logger.h"
#include "vulkan/Draw.h"
#include "vulkan/VulkanManager.h"
#include "vulkan/texture_manager.h"

#include "area_render_state.h"

/* Render Object
 * quadHandles: array<int>
 * 
*/

const int renderHandleInvalid = -1;

static Heap inactiveRenderHandles = { };

static int renderObjectQuadHandles[NUM_RENDER_OBJECT_SLOTS][MAX_NUM_RENDER_OBJECT_QUADS];

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

int loadRenderObject(const String textureID, const BoxF quadDimensions, const int numQuads, const Vector3D quadPositions[static numQuads]) {
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
		loadModel(modelPoolMain, quadPosition, quadDimensions, textureID, &quadHandle);
		renderObjectQuadHandles[renderHandle][i] = quadHandle;
	}
	
	return renderHandle;
}

void unloadRenderObject(int *const pRenderHandle) {
	if (!pRenderHandle) {
		return;
	} else if (!validateRenderObjectHandle(*pRenderHandle)) {
		return;
	}
	
	/*for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		if (validateQuadHandle(renderObjectQuadHandles[*pRenderHandle][i])) {
			unloadQuad(&renderObjectQuadHandles[*pRenderHandle][i]);
		}
	}*/
	
	for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		if (renderObjectQuadHandles[*pRenderHandle][i] >= 0) {
			unloadModel(modelPoolMain, &renderObjectQuadHandles[*pRenderHandle][i]);
		}
	}
	
	heapPush(&inactiveRenderHandles, pRenderHandle);
	*pRenderHandle = renderHandleInvalid;
}

bool validateRenderObjectHandle(const int renderHandle) {
	return renderHandle >= 0 && renderHandle < (int)numRenderObjectSlots;
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
	//return setQuadTranslation(renderObjectQuadHandles[renderHandle][quadIndex], translation);
	modelSetTranslation(modelPoolMain, renderObjectQuadHandles[renderHandle][quadIndex], translation);
	return true;
}

int renderObjectGetTextureHandle(const int renderHandle, const int quadIndex) {
	if (!validateRenderObjectHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return -1;
	}
	
	/*const int quadHandle = renderObjectQuadHandles[renderHandle][quadIndex];
	TextureState *const pTextureState = getQuadTextureState(quadHandle);
	if (pTextureState) {
		return pTextureState->textureHandle;
	}
	return -1;*/
	
	const int quadHandle = renderObjectQuadHandles[renderHandle][quadIndex];
	TextureState *const pTextureState = modelGetTextureState(modelPoolMain, quadHandle);
	if (pTextureState) {
		return pTextureState->textureHandle;
	}
	return -1;
}

bool renderObjectAnimate(const int renderHandle) {
	if (!validateRenderObjectHandle(renderHandle)) {
		return false;
	}
	
	for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		const int quadHandle = renderObjectQuadHandles[renderHandle][i];
		if (quadHandle < 0) {
			continue;
		}
		
		TextureState *const pTextureState = modelGetTextureState(modelPoolMain, quadHandle);
		if (textureStateAnimate(pTextureState) == 2) {
			const unsigned int imageIndex = pTextureState->startCell + pTextureState->currentFrame;
			//updateDrawData(quadHandle, imageIndex);
			updateDrawInfo(modelPoolMain, quadHandle, imageIndex);
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