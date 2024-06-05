#include "render_object.h"

#include <stddef.h>

#include <DataStuff/Heap.h>

#include "log/logging.h"
#include "vulkan/vulkan_render.h"
#include "vulkan/texture_manager.h"
#include "util/bit.h"

#include "area_render_state.h"

const int renderHandleInvalid = -1;

static Heap inactiveRenderHandles = { 0 };

int renderObjQuadIDs[NUM_RENDER_OBJECT_SLOTS];
RenderTransform renderObjTransforms[NUM_RENDER_OBJECT_SLOTS];
TextureState renderObjTextureStates[NUM_RENDER_OBJECT_SLOTS];

bool initRenderObjectManager(void) {
	inactiveRenderHandles = newHeap(num_render_object_slots, sizeof(int), cmpFuncInt);
	if (!heapValidate(inactiveRenderHandles)) {
		deleteHeap(&inactiveRenderHandles);
		return false;
	}
	for (int i = 0; i < (int)num_render_object_slots; ++i) {
		heapPush(&inactiveRenderHandles, &i);
	}
	return true;
}

bool terminateRenderObjectManager(void) {
	deleteHeap(&inactiveRenderHandles);
	return true;
}

int loadRenderObject(const DimensionsF quadDimensions, const Transform transform, const String textureID) {
	int renderHandle = renderHandleInvalid;
	heapPop(&inactiveRenderHandles, &renderHandle);
	if (!validateRenderHandle(renderHandle)) {
		return renderHandle;
	}
	
	renderObjTextureStates[renderHandle] = newTextureState(textureID);
	
	const int quadID = loadQuad(quadDimensions, renderObjTextureStates[renderHandle].textureHandle);
	if (!validateQuadID(quadID)) {
		unloadRenderObject(&renderHandle);
		return renderHandle;
	}
	renderObjQuadIDs[renderHandle] = quadID;
	
	render_vector_reset(&renderObjTransforms[renderHandle].translation, transform.translation);
	render_vector_reset(&renderObjTransforms[renderHandle].scaling, transform.scaling);
	render_vector_reset(&renderObjTransforms[renderHandle].rotation, transform.rotation);
	
	return renderHandle;
}

void unloadRenderObject(int *const pRenderHandle) {
	if (!pRenderHandle) {
		return;
	} else if (validateRenderHandle(*pRenderHandle)) {
		return;
	}
	heapPush(&inactiveRenderHandles, pRenderHandle);
	*pRenderHandle = renderHandleInvalid;
}

bool validateRenderHandle(const int renderHandle) {
	return renderHandle >= 0 && renderHandle < (int)num_render_object_slots;
}

RenderTransform *getRenderObjTransform(const int renderHandle) {
	if (!validateRenderHandle(renderHandle)) {
		logf_message(ERROR, "Error getting render object transform: render object handle (%i) is invalid.", renderHandle);
		return nullptr;
	}
	return &renderObjTransforms[renderHandle];
}

TextureState *getRenderObjTexState(const int renderHandle) {
	if (!validateRenderHandle(renderHandle)) {
		logf_message(ERROR, "Error getting render object texture state: render object handle (%i) is invalid.", renderHandle);
		return nullptr;
	}
	return &renderObjTextureStates[renderHandle];
}

unsigned int renderObjectGetAnimation(const int renderHandle) {
	if (!validateRenderHandle(renderHandle)) {
		logf_message(ERROR, "Error accessing render object: render object handle (%i) is invalid.", renderHandle);
		return 0;
	}
	return renderObjTextureStates[renderHandle].currentAnimation;
}

bool renderObjectSetAnimation(const int renderHandle, const unsigned int nextAnimation) {
	if (!validateRenderHandle(renderHandle)) {
		logf_message(ERROR, "Error accessing render object: render object handle (%i) is invalid.", renderHandle);
		return false;
	}
	return textureStateSetAnimation(&renderObjTextureStates[renderHandle], nextAnimation);
}
