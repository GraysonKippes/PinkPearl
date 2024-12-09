#include "RenderManager.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include "config.h"
#include "log/Logger.h"
#include "util/allocate.h"
#include "vulkan/Draw.h"
#include "vulkan/VulkanManager.h"
#include "vulkan/texture_manager.h"

#define DATA_PATH (RESOURCE_PATH "data/")
#define FGT_PATH (RESOURCE_PATH "data/textures.fgt")

#define RENDER_OBJECT_MAX_COUNT 64
#define RENDER_OBJECT_QUAD_MAX_COUNT 64

typedef struct RenderObjectQuad {
	int32_t handle;
	ModelPool modelPool;
} RenderObjectQuad;

typedef struct RenderObject {
	
	// True if this render object is 'loaded' or in use.
	bool active;
	
	int32_t textureHandle;
	
	// Array of handles to quads being rendered.
	int32_t quadCount;
	RenderObjectQuad *pQuads;
	
} RenderObject;

static RenderObject renderObjects[RENDER_OBJECT_MAX_COUNT];

const Vector4F COLOR_WHITE 	= { 1.0F, 1.0F, 1.0F, 1.0F };
const Vector4F COLOR_BLACK 	= { 0.0F, 0.0F, 0.0F, 1.0F };
const Vector4F COLOR_RED 	= { 1.0F, 0.0F, 0.0F, 1.0F };
const Vector4F COLOR_GREEN 	= { 0.0F, 1.0F, 0.0F, 1.0F };
const Vector4F COLOR_BLUE 	= { 0.0F, 0.0F, 1.0F, 1.0F };
const Vector4F COLOR_YELLOW	= { 1.0F, 1.0F, 0.0F, 1.0F };
const Vector4F COLOR_TEAL 	= { 0.0F, 1.0F, 1.0F, 1.0F };
const Vector4F COLOR_PURPLE	= { 1.0F, 0.0F, 1.0F, 1.0F };
const Vector4F COLOR_PINK	= { 1.0F, 0.6392156863F, 0.7568627451F, 1.0F };

void initRenderManager(void) {
	
	initVulkanManager();
	initTextureManager();

	TexturePack texturePack = readTexturePackFile(FGT_PATH);
	textureManagerLoadTexturePack(texturePack);
	deleteTexturePack(&texturePack);
	
	// TEMPORARY
	// Create room textures -- one for each room size.
	for (int i = 0; i < (int)num_room_sizes; ++i) {
		
		// Give each room texture enough layers for each room layer (background, foreground) and each room cache slot.
		TextureCreateInfo roomTextureCreateInfo = (TextureCreateInfo){
			.textureID = makeNullString(),
			.isLoaded = false,
			.isTilemap = false,
			.numCells.width = 1,
			.numCells.length = numRoomLayers * numRoomTextureCacheSlots,
			.cellExtent.width = 16,
			.cellExtent.length = 16,
			.numAnimations = 4,
			.animations = (TextureAnimation[4]){
				{
					.startCell = 0,
					.numFrames = 1,
					.framesPerSecond = 0
				},
				{
					.startCell = 1,
					.numFrames = 1,
					.framesPerSecond = 0
				},
				{
					.startCell = 2,
					.numFrames = 1,
					.framesPerSecond = 0
				},
				{
					.startCell = 3,
					.numFrames = 1,
					.framesPerSecond = 0
				}
			}
		};
		
		switch((RoomSize)i) {
			case ROOM_SIZE_XS:
				roomTextureCreateInfo.textureID = makeStaticString("roomXS");
				roomTextureCreateInfo.cellExtent.width = 8 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 5 * tile_texel_length;
				break;
			case ROOM_SIZE_S:
				roomTextureCreateInfo.textureID = makeStaticString("roomS");
				roomTextureCreateInfo.cellExtent.width = 16 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 10 * tile_texel_length;
				break;
			case ROOM_SIZE_M:
				roomTextureCreateInfo.textureID = makeStaticString("roomM");
				roomTextureCreateInfo.cellExtent.width = 24 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 15 * tile_texel_length;
				break;
			case ROOM_SIZE_L:
				roomTextureCreateInfo.textureID = makeStaticString("roomL");
				roomTextureCreateInfo.cellExtent.width = 32 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 20 * tile_texel_length;
				break;
		}
		
		textureManagerLoadTexture(roomTextureCreateInfo);
	}
}

void terminateRenderManager(void) {
	terminateVulkanManager();
}

void tickRenderManager(void) {
	for (int32_t handle = 0; handle < RENDER_OBJECT_MAX_COUNT; ++handle) {
		if (!renderObjects[handle].active) {
			continue;
		}
		for (int32_t quadIndex = 0; quadIndex < renderObjects[handle].quadCount; ++quadIndex) {
			if (renderObjects[handle].pQuads[quadIndex].handle >= 0) {
				modelSettleTransform(renderObjects[handle].pQuads[quadIndex].modelPool, renderObjects[handle].pQuads[quadIndex].handle);
			}
		}
	}
}

void renderFrame(const float timeDelta, const Vector4F cameraPosition, const ProjectionBounds projectionBounds, const bool animate) {
	glfwPollEvents();
	
	if (animate) {
		for (int32_t i = 0; i < RENDER_OBJECT_MAX_COUNT; ++i) {
			renderObjectAnimate(i);
		}
	}
	
	drawFrame(timeDelta, cameraPosition, projectionBounds);
}

int32_t loadRenderObject(const RenderObjectLoadInfo loadInfo) {
	if (loadInfo.quadCount <= 0 || loadInfo.quadCount > RENDER_OBJECT_QUAD_MAX_COUNT) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error loading render object: quad count (%i) is invalid.", loadInfo.quadCount);
		return -1;
	}
	
	int32_t handle = -1;
	for (int32_t i = 0; i < RENDER_OBJECT_MAX_COUNT; ++i) {
		if (!renderObjects[i].active) {
			handle = i;
			break;
		}
	}
	if (!validateRenderObjectHandle(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error loading render object: no render object slots available.");
		return -1;
	}
	
	renderObjects[handle].quadCount = loadInfo.quadCount;
	renderObjects[handle].pQuads = heapAlloc(loadInfo.quadCount, sizeof(RenderObjectQuad));
	if (!renderObjects[handle].pQuads) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error loading render object: failed to allocate quad array.");
		return -1;
	}
	for (int32_t i = 0; i < loadInfo.quadCount; ++i) {
		renderObjects[handle].pQuads[i] = (RenderObjectQuad){
			.handle = -1,
			.modelPool = nullptr
		};
	}
	
	if (!stringIsNull(loadInfo.textureID)) {
		renderObjects[handle].textureHandle = findTexture(loadInfo.textureID);
	} else {
		renderObjects[handle].textureHandle = textureHandleMissing;
	}
	const Texture texture = getTexture(renderObjects[handle].textureHandle);
	
	for (int32_t quadIndex = 0; quadIndex < loadInfo.quadCount; ++quadIndex) {
		
		const QuadLoadInfo quadLoadInfo = loadInfo.pQuadLoadInfos[quadIndex];
		
		int32_t imageIndex = 0;
		if (quadLoadInfo.initAnimation >= 0 && quadLoadInfo.initAnimation < (int32_t)texture.numAnimations) {
			imageIndex = texture.animations[quadLoadInfo.initAnimation].startCell + quadLoadInfo.initCell;
		}
		
		const ModelLoadInfo modelLoadInfo = {
			.modelPool = quadLoadInfo.quadType == QUAD_TYPE_WIREFRAME ? modelPoolDebug : modelPoolMain,
			.position = vec3DtoVec4F(quadLoadInfo.initPosition),
			.dimensions = quadLoadInfo.quadDimensions,
			.cameraFlag = quadLoadInfo.quadType == QUAD_TYPE_GUI ? 0 : 1,
			.textureHandle = renderObjects[handle].textureHandle,
			.color = quadLoadInfo.color,
			.imageIndex = imageIndex
		};
		loadModel(modelLoadInfo, &renderObjects[handle].pQuads[quadIndex].handle);
		renderObjects[handle].pQuads[quadIndex].modelPool = modelLoadInfo.modelPool;
		logMsg(loggerRender, LOG_LEVEL_VERBOSE, "Loaded quad %i in render object %i (model handle = %i).", quadIndex, handle, renderObjects[handle].pQuads[quadIndex].handle);
	}
	
	renderObjects[handle].active = true;
	logMsg(loggerRender, LOG_LEVEL_VERBOSE, "Loaded render object %i.", handle);
	return handle;
}

void unloadRenderObject(int32_t *const pHandle) {
	if (!pHandle) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error unloading render object: pointer to render object handle is null.");
		return;
	} else if (!renderObjectExists(*pHandle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error unloading render object: render object %i does not exist.", *pHandle);
		return;
	}
	
	for (int32_t quadIndex = 0; quadIndex < renderObjects[*pHandle].quadCount; ++quadIndex) {
		if (renderObjects[*pHandle].pQuads[quadIndex].handle >= 0) {
			unloadModel(renderObjects[*pHandle].pQuads[quadIndex].modelPool, &renderObjects[*pHandle].pQuads[quadIndex].handle);
		}
	}
	renderObjects[*pHandle].active = false;
	renderObjects[*pHandle].quadCount = 0;
	renderObjects[*pHandle].pQuads = heapFree(renderObjects[*pHandle].pQuads);
	
	logMsg(loggerRender, LOG_LEVEL_VERBOSE, "Unloaded render object %i.", *pHandle);
	*pHandle = -1;
}

int32_t loadRenderText(const String text, const Vector3D position, const Vector4F color) {
	
	QuadLoadInfo quadLoadInfos[text.length] = { };
	for (int32_t i = 0; i < (int32_t)text.length; ++i) {
		quadLoadInfos[i] = (QuadLoadInfo){
			.quadType = QUAD_TYPE_GUI,
			.initPosition = addVec(position, mulVec3D(makeVec3D(0.5, 0.0, 0.0), (double)i)),
			.quadDimensions = (BoxF){ .x1 = -0.25F, .y1 = -0.25F, .x2 = 0.25F, .y2 = 0.25F },
			.initAnimation = 0,
			.initCell = (int32_t)text.pBuffer[i],
			.color = color
		};
	}
	
	const RenderObjectLoadInfo loadInfo = {
		.textureID = makeStaticString("gui/fontFrogBlock"),
		.quadCount = text.length,
		.pQuadLoadInfos = quadLoadInfos
	};
	
	return loadRenderObject(loadInfo);
}

void writeRenderText(const int32_t handle, const char *const pFormat, ...) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error writing render text: render object %i does not exist.", handle);
	}
	
	#define BUFSIZE (RENDER_OBJECT_QUAD_MAX_COUNT + 1)
	char buffer[BUFSIZE];
	memset(buffer, 0, BUFSIZE);
	va_list vlist;
	va_start(vlist, pFormat);
	vsnprintf_s(buffer, BUFSIZE, renderObjects[handle].quadCount, pFormat, vlist);
	va_end(vlist);
	#undef BUFSIZE
	
	for (int32_t quadIndex = 0; quadIndex < renderObjects[handle].quadCount; ++quadIndex) {
		TextureState *const pTextureState = modelGetTextureState(renderObjects[handle].pQuads[quadIndex].modelPool, renderObjects[handle].pQuads[quadIndex].handle);
		if (pTextureState) {
			const unsigned int imageIndex = (unsigned int)buffer[quadIndex];
			pTextureState->currentFrame = imageIndex;
			updateDrawInfo(renderObjects[handle].pQuads[quadIndex].modelPool, renderObjects[handle].pQuads[quadIndex].handle, imageIndex);
		}
	}
}

bool validateRenderObjectHandle(const int32_t handle) {
	return handle >= 0 && handle < RENDER_OBJECT_MAX_COUNT;
}

bool renderObjectExists(const int32_t handle) {
	return validateRenderObjectHandle(handle) && renderObjects[handle].active;
}

int32_t renderObjectLoadQuad(const int32_t handle, const QuadLoadInfo loadInfo) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error loading render object quad: render object %i does not exist.", handle);
		return -1;
	}
	
	// Find the first appropriate slot to use.
	bool enoughSpace = false; // Set to true if space for the new quad is found.
	int32_t quadIndex = 0;
	for (; quadIndex < renderObjects[handle].quadCount; ++quadIndex) {
		if (renderObjects[handle].pQuads[quadIndex].handle < 0) {
			enoughSpace = true;
			break;
		}
	}
	
	// If there is not enough space for the new quad, double the amount of space.
	if (!enoughSpace) {
		const int32_t newCapacity = renderObjects[handle].quadCount > 0 ? 2 * renderObjects[handle].quadCount : 1;
		RenderObjectQuad *const pRealloc = heapRealloc(renderObjects[handle].pQuads, newCapacity, sizeof(RenderObjectQuad));
		if (!pRealloc) {
			logMsg(loggerRender, LOG_LEVEL_ERROR, "Error loading render object quad: failed to reallocate quad array.");
			return -1;
		}
		renderObjects[handle].pQuads = pRealloc;
		quadIndex = renderObjects[handle].quadCount;
		renderObjects[handle].quadCount = newCapacity;
		for (int32_t i = quadIndex; i < newCapacity; ++i) {
			renderObjects[handle].pQuads[i].handle = -1;
		}
	}
	
	const Texture texture = getTexture(renderObjects[handle].textureHandle);
	int32_t imageIndex = 0;
	if (loadInfo.initAnimation >= 0 && loadInfo.initAnimation < (int32_t)texture.numAnimations) {
		imageIndex = texture.animations[loadInfo.initAnimation].startCell + loadInfo.initCell;
	}
	
	const ModelLoadInfo modelLoadInfo = {
		.modelPool = loadInfo.quadType == QUAD_TYPE_WIREFRAME ? modelPoolDebug : modelPoolMain,
		.position = vec3DtoVec4F(loadInfo.initPosition),
		.dimensions = loadInfo.quadDimensions,
		.cameraFlag = loadInfo.quadType == QUAD_TYPE_GUI ? 0 : 1,
		.textureHandle = renderObjects[handle].textureHandle,
		.color = loadInfo.color,
		.imageIndex = imageIndex
	};
	loadModel(modelLoadInfo, &renderObjects[handle].pQuads[quadIndex].handle);
	renderObjects[handle].pQuads[quadIndex].modelPool = modelLoadInfo.modelPool;
	
	logMsg(loggerRender, LOG_LEVEL_VERBOSE, "Loaded quad %i in render object %i (model handle = %i).", quadIndex, handle, renderObjects[handle].pQuads[quadIndex].handle);
	return quadIndex;
}

void renderObjectUnloadQuad(const int32_t handle, int32_t *const pQuadIndex) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error unloading render object quad: render object %i does not exist.", handle);
		return;
	} else if (!renderObjectQuadExists(handle, *pQuadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error unloading render object quad: quad %i of render object %i does not exist.", *pQuadIndex, handle);
		return;
	}
	unloadModel(renderObjects[handle].pQuads[*pQuadIndex].modelPool, &renderObjects[handle].pQuads[*pQuadIndex].handle);
	renderObjects[handle].pQuads[*pQuadIndex].modelPool = nullptr;
	*pQuadIndex = -1;
}

bool validateRenderObjectQuadIndex(const int32_t quadIndex) {
	return quadIndex >= 0 && quadIndex < RENDER_OBJECT_QUAD_MAX_COUNT;
}

bool renderObjectQuadExists(const int32_t handle, const int32_t quadIndex) {
	return renderObjectExists(handle) && validateRenderObjectQuadIndex(quadIndex) && renderObjects[handle].pQuads[quadIndex].handle >= 0;
}

void renderObjectSetPosition(const int32_t handle, const int32_t quadIndex, const Vector3D position) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error setting render object position: render object %i does not exist.", handle);
		return;
	} else if (!renderObjectQuadExists(handle, quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error setting render object position: quad %i of render object %i does not exist.", quadIndex, handle);
		return;
	}
	modelSetTranslation(renderObjects[handle].pQuads[quadIndex].modelPool, renderObjects[handle].pQuads[quadIndex].handle, vec3DtoVec4F(position));
}

int32_t renderObjectGetTextureHandle(const int32_t handle, const int32_t quadIndex) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error getting render object texture handle: render object %i does not exist.", handle);
		return -1;
	} else if (!renderObjectQuadExists(handle, quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error getting render object texture handle: quad %i of render object %i does not exist.", quadIndex, handle);
		return -1;
	}
	return renderObjects[handle].textureHandle;
}

void renderObjectAnimate(const int32_t handle) {
	if (!renderObjectExists(handle)) {
		//logMsg(loggerRender, LOG_LEVEL_ERROR, "Error animating render object: render object %i does not exist.", handle);
		return;
	}
	
	for (int32_t quadIndex = 0; quadIndex < renderObjects[handle].quadCount; ++quadIndex) {
		if (renderObjects[handle].pQuads[quadIndex].handle < 0) {
			continue;
		}
		
		TextureState *const pTextureState = modelGetTextureState(renderObjects[handle].pQuads[quadIndex].modelPool, renderObjects[handle].pQuads[quadIndex].handle);
		if (textureStateAnimate(pTextureState) == 2) {
			const uint32_t imageIndex = pTextureState->startCell + pTextureState->currentFrame;
			updateDrawInfo(renderObjects[handle].pQuads[quadIndex].modelPool, renderObjects[handle].pQuads[quadIndex].handle, imageIndex);
		}
	}
}

uint32_t renderObjectGetAnimation(const int32_t handle, const int32_t quadIndex) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error getting render object animation: render object %i does not exist.", handle);
		return 0;
	} else if (!renderObjectQuadExists(handle, quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error getting render object animation: quad %i of render object %i does not exist.", quadIndex, handle);
		return 0;
	}
	
	const TextureState *const pTextureState = modelGetTextureState(renderObjects[handle].pQuads[quadIndex].modelPool, renderObjects[handle].pQuads[quadIndex].handle);
	if (pTextureState) {
		return pTextureState->currentAnimation;
	}
	return 0;
}

bool renderObjectSetAnimation(const int32_t handle, const int32_t quadIndex, const uint32_t nextAnimation) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error setting render object animation: render object %i does not exist.", handle);
		return false;
	} else if (!renderObjectQuadExists(handle, quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error setting render object animation: quad %i of render object %i does not exist.", quadIndex, handle);
		return false;
	}
	
	TextureState *const pTextureState = modelGetTextureState(renderObjects[handle].pQuads[quadIndex].modelPool, renderObjects[handle].pQuads[quadIndex].handle);
	if (pTextureState) {
		if (!textureStateSetAnimation(pTextureState, nextAnimation)) {
			return false;
		}
		const uint32_t imageIndex = pTextureState->startCell + pTextureState->currentFrame;
		updateDrawInfo(renderObjects[handle].pQuads[quadIndex].modelPool, renderObjects[handle].pQuads[quadIndex].handle, imageIndex);
		return true;
	}
	return false;
}