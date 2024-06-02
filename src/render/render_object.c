#include "render_object.h"

#include <stddef.h>

#include <DataStuff/Heap.h>

#include "log/logging.h"
#include "vulkan/vulkan_render.h"
#include "vulkan/texture_manager.h"
#include "util/bit.h"

#include "area_render_state.h"

const int renderHandleInvalid = -1;

static Heap inactive_render_handles = { 0 };

int renderObjQuadIDs[NUM_RENDER_OBJECT_SLOTS];
RenderTransform renderObjTransforms[NUM_RENDER_OBJECT_SLOTS];
TextureState renderObjTextureStates[NUM_RENDER_OBJECT_SLOTS];

bool init_render_object_manager(void) {
	inactive_render_handles = newHeap(num_render_object_slots, sizeof(int), cmpFuncInt);
	if (!heapValidate(inactive_render_handles)) {
		deleteHeap(&inactive_render_handles);
		return false;
	}
	for (int i = 0; i < (int)num_render_object_slots; ++i) {
		heapPush(&inactive_render_handles, &i);
	}
	return true;
}

int loadRenderObject(const DimensionsF quadDimensions, const Transform transform, const String textureID) {
	int renderHandle = renderHandleInvalid;
	heapPop(&inactive_render_handles, &renderHandle);
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
	if (pRenderHandle == NULL) {
		return;
	} else if (validateRenderHandle(*pRenderHandle)) {
		return;
	}
	heapPush(&inactive_render_handles, pRenderHandle);
	*pRenderHandle = renderHandleInvalid;
}

bool validateRenderHandle(const int render_handle) {
	return render_handle >= 0 && render_handle < (int)num_render_object_slots;
}

RenderTransform *getRenderObjTransform(const int renderHandle) {
	if (!validateRenderHandle(renderHandle)) {
		logf_message(ERROR, "Error getting render object transform: render object handle (%u) is invalid.", renderHandle);
		return NULL;
	}
	return &renderObjTransforms[renderHandle];
}

TextureState *getRenderObjTexState(const int renderHandle) {
	if (!validateRenderHandle(renderHandle)) {
		logf_message(ERROR, "Error getting render object texture state: render object handle (%u) is invalid.", renderHandle);
		return NULL;
	}
	return &renderObjTextureStates[renderHandle];
}
