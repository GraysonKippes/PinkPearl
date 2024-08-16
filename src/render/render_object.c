#include "render_object.h"

#include <stddef.h>

#include <DataStuff/Heap.h>

#include "log/Logger.h"
#include "vulkan/vulkan_render.h"
#include "vulkan/texture_manager.h"

#include "area_render_state.h"

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
	
	const TextureState quadTextureState = newTextureState(textureID);
	for (int i = 0; i < numQuads; ++i) {
		const Vector4F quadPosition = {
			.x = (float)quadPositions[i].x,
			.y = (float)quadPositions[i].y,
			.z = (float)quadPositions[i].z,
			.w = 1.0F
		};
		
		const int quadHandle = loadQuad(quadDimensions, quadPosition, quadTextureState);
		if (!validateQuadHandle(quadHandle)) {
			unloadRenderObject(&renderHandle);
			return renderHandleInvalid;
		}
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
	
	for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		if (validateQuadHandle(renderObjectQuadHandles[*pRenderHandle][i])) {
			unloadQuad(&renderObjectQuadHandles[*pRenderHandle][i]);
		}
	}
	
	heapPush(&inactiveRenderHandles, pRenderHandle);
	*pRenderHandle = renderHandleInvalid;
}

bool validateRenderObjectHandle(const int renderHandle) {
	return renderHandle >= 0 && renderHandle < (int)numRenderObjectSlots;
}

int renderObjectLoadQuad(const int renderObjectHandle, const BoxF quadDimensions, const Vector3D quadPosition, const TextureState textureState) {
	if (!validateRenderObjectHandle(renderObjectHandle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error loading render object quad: render object handle (%i) is invalid.", renderObjectHandle);
		return -1;
	}
	
	int quadIndex = -1;
	for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		if (!validateQuadHandle(renderObjectQuadHandles[renderObjectHandle][i])) {
			quadIndex = i;
		}
	}
	
	if (!validateRenderObjectQuadIndex(quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error loading render object quad: failed to find space in render object's quad array.");
		return quadIndex;
	}
	
	Vector4F position = {
		.x = (float)quadPosition.x,
		.y = (float)quadPosition.y,
		.z = (float)quadPosition.z,
		.w = 1.0F
	};
	
	int quadHandle = loadQuad(quadDimensions, position, textureState);
	if (!validateQuadHandle(quadHandle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error loading render object quad: failed to load quad.");
		return -1;
	}
	renderObjectQuadHandles[renderObjectHandle][quadIndex] = quadHandle;
	
	return quadIndex;
}

void renderObjectUnloadQuad(const int renderObjectHandle, const int quadIndex) {
	if (!validateRenderObjectHandle(renderObjectHandle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error unloading render object quad: render object handle (%i) is invalid.", renderObjectHandle);
		return;
	} else if (!validateRenderObjectQuadIndex(quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error unloading render object quad: quad index (%i) is invalid.", quadIndex);
		return;
	}
	unloadQuad(&renderObjectQuadHandles[renderObjectHandle][quadIndex]);
}

bool renderObjectQuadExists(const int renderObjectHandle, const int quadIndex) {
	if (!validateRenderObjectHandle(renderObjectHandle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error checking render object quad's existence: render object handle (%i) is invalid.", renderObjectHandle);
		return false;
	} else if (!validateRenderObjectQuadIndex(quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error checking render object quad's existence: quad index (%i) is invalid.", quadIndex);
		return false;
	}
	return validateQuadHandle(renderObjectQuadHandles[renderObjectHandle][quadIndex]);
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
	return setQuadTranslation(renderObjectQuadHandles[renderHandle][quadIndex], translation);
}

int renderObjectGetTextureHandle(const int renderHandle, const int quadIndex) {
	if (!validateRenderObjectHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return -1;
	}
	
	const int quadHandle = renderObjectQuadHandles[renderHandle][quadIndex];
	TextureState *const pTextureState = getQuadTextureState(quadHandle);
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
		if (!validateQuadHandle(quadHandle)) {
			continue;
		}
		
		TextureState *const pTextureState = getQuadTextureState(quadHandle);
		if (textureStateAnimate(pTextureState) == 2) {
			const unsigned int imageIndex = pTextureState->startCell + pTextureState->currentFrame;
			updateDrawData(quadHandle, imageIndex);
		}
	}
	
	return true;
}

unsigned int renderObjectGetAnimation(const int renderHandle, const int quadIndex) {
	if (!validateRenderObjectHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return 0;
	}
	
	const int quadHandle = renderObjectQuadHandles[renderHandle][quadIndex];
	TextureState *const pTextureState = getQuadTextureState(quadHandle);
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
	TextureState *const pTextureState = getQuadTextureState(quadHandle);
	if (pTextureState) {
		if (!textureStateSetAnimation(pTextureState, nextAnimation)) {
			return false;
		}
		const unsigned int imageIndex = pTextureState->startCell + pTextureState->currentFrame;
		updateDrawData(quadHandle, imageIndex);
		return true;
	}
	return false;
}