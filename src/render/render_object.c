#include "render_object.h"

#include <stddef.h>

#include <DataStuff/Heap.h>

#include "log/logging.h"
#include "vulkan/vulkan_render.h"
#include "vulkan/texture_manager.h"
#include "util/bit.h"

#include "area_render_state.h"

const int render_handle_invalid = -1;
const int render_handle_dangling = -2;

Heap inactive_render_handles = { 0 };

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

int loadRenderObject(const DimensionsF quadDimensions, const transform_t transform, const string_t textureID) {
	int renderHandle = render_handle_invalid;
	heapPop(&inactive_render_handles, &renderHandle);
	
	if (validate_render_handle(renderHandle)) {
		const int quadID = loadQuad(quadDimensions);
		if (!validateQuadID(quadID)) {
			unloadRenderObject(&renderHandle);
			return renderHandle;
		}
		renderObjQuadIDs[renderHandle] = quadID;
		
		render_vector_reset(&renderObjTransforms[renderHandle].translation, transform.translation);
		render_vector_reset(&renderObjTransforms[renderHandle].scaling, transform.scaling);
		render_vector_reset(&renderObjTransforms[renderHandle].rotation, transform.rotation);
		
		renderObjTextureStates[renderHandle] = newTextureState(textureID);
	}
	return renderHandle;
}

void unloadRenderObject(int *const pRenderHandle) {
	if (pRenderHandle == NULL) {
		return;
	} else if (validate_render_handle(*pRenderHandle)) {
		return;
	}
	heapPush(&inactive_render_handles, pRenderHandle);
	*pRenderHandle = render_handle_dangling;
}

bool validate_render_handle(const int render_handle) {
	return render_handle >= 0 && render_handle < (int)num_render_object_slots;
}

RenderTransform *getRenderObjTransform(const int renderHandle) {
	if (!validate_render_handle(renderHandle)) {
		logf_message(ERROR, "Error getting render object transform: render object handle (%u) is invalid.", renderHandle);
		return NULL;
	}
	return &renderObjTransforms[renderHandle];
}

TextureState *getRenderObjTexState(const int renderHandle) {
	if (!validate_render_handle(renderHandle)) {
		logf_message(ERROR, "Error getting render object texture state: render object handle (%u) is invalid.", renderHandle);
		return NULL;
	}
	return &renderObjTextureStates[renderHandle];
}
