#include "render_object.h"

#include <stddef.h>

#include <DataStuff/Heap.h>

#include "log/Logger.h"
#include "vulkan/vulkan_render.h"
#include "vulkan/texture_manager.h"

#include "area_render_state.h"

const int renderHandleInvalid = -1;

static Heap inactiveRenderHandles = { };

int renderObjQuadIDs[NUM_RENDER_OBJECT_SLOTS][MAX_NUM_RENDER_OBJECT_QUADS];

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
	if (!validateRenderHandle(renderHandle)) {
		return renderHandleInvalid;
	}
	
	for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		renderObjQuadIDs[renderHandle][i] = -1;
	}
	
	const TextureState quadTextureState = newTextureState(textureID);
	for (int i = 0; i < numQuads; ++i) {
		const Vector4F quadPosition = {
			.x = (float)quadPositions[i].x,
			.y = (float)quadPositions[i].y,
			.z = (float)quadPositions[i].z,
			.w = 1.0F
		};
		
		const int quadID = loadQuad(quadDimensions, quadPosition, quadTextureState);
		if (!validateQuadID(quadID)) {
			unloadRenderObject(&renderHandle);
			return renderHandleInvalid;
		}
		renderObjQuadIDs[renderHandle][i] = quadID;
	}
	
	return renderHandle;
}

void unloadRenderObject(int *const pRenderHandle) {
	if (!pRenderHandle) {
		return;
	} else if (!validateRenderHandle(*pRenderHandle)) {
		return;
	}
	
	for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		if (validateQuadID(renderObjQuadIDs[*pRenderHandle][i])) {
			unloadQuad(&renderObjQuadIDs[*pRenderHandle][i]);
		}
	}
	
	heapPush(&inactiveRenderHandles, pRenderHandle);
	*pRenderHandle = renderHandleInvalid;
}

bool validateRenderHandle(const int renderHandle) {
	return renderHandle >= 0 && renderHandle < (int)numRenderObjectSlots;
}

bool validateRenderObjectQuadIndex(const int quadIndex) {
	return quadIndex >= 0 && quadIndex < (int)maxNumRenderObjectQuads;
}

bool renderObjectSetPosition(const int renderHandle, const int quadIndex, const Vector3D position) {
	if (!validateRenderHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return false;
	}
	const Vector4F translation = {
		.x = (float)position.x,
		.y = (float)position.y,
		.z = (float)position.z,
		.w = 1.0F
	};
	return setQuadTranslation(renderObjQuadIDs[renderHandle][quadIndex], translation);
}

int renderObjectGetTextureHandle(const int renderHandle, const int quadIndex) {
	if (!validateRenderHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return -1;
	}
	
	const int quadID = renderObjQuadIDs[renderHandle][quadIndex];
	TextureState *const pTextureState = getQuadTextureState(quadID);
	if (pTextureState) {
		return pTextureState->textureHandle;
	}
	return -1;
}

bool renderObjectAnimate(const int renderHandle) {
	if (!validateRenderHandle(renderHandle)) {
		return false;
	}
	
	for (int i = 0; i < maxNumRenderObjectQuads; ++i) {
		const int quadID = renderObjQuadIDs[renderHandle][i];
		if (!validateQuadID(quadID)) {
			continue;
		}
		
		TextureState *const pTextureState = getQuadTextureState(quadID);
		if (textureStateAnimate(pTextureState) == 2) {
			const unsigned int imageIndex = pTextureState->startCell + pTextureState->currentFrame;
			updateDrawData(quadID, imageIndex);
		}
	}
	
	return true;
}

unsigned int renderObjectGetAnimation(const int renderHandle, const int quadIndex) {
	if (!validateRenderHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return 0;
	}
	
	const int quadID = renderObjQuadIDs[renderHandle][quadIndex];
	TextureState *const pTextureState = getQuadTextureState(quadID);
	if (pTextureState) {
		return pTextureState->currentAnimation;
	}
	return 0;
}

bool renderObjectSetAnimation(const int renderHandle, const int quadIndex, const unsigned int nextAnimation) {
	if (!validateRenderHandle(renderHandle) || !validateRenderObjectQuadIndex(quadIndex)) {
		return false;
	}
	
	const int quadID = renderObjQuadIDs[renderHandle][quadIndex];
	TextureState *const pTextureState = getQuadTextureState(quadID);
	if (pTextureState) {
		if (!textureStateSetAnimation(pTextureState, nextAnimation)) {
			return false;
		}
		const unsigned int imageIndex = pTextureState->startCell + pTextureState->currentFrame;
		updateDrawData(quadID, imageIndex);
		return true;
	}
	return false;
}