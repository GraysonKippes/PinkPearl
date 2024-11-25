#include "RenderManager.h"

#include <assert.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>

#include "config.h"
#include "log/Logger.h"
#include "vulkan/Draw.h"
#include "vulkan/VulkanManager.h"
#include "vulkan/texture_manager.h"

#define DATA_PATH (RESOURCE_PATH "data/")
#define FGT_PATH (RESOURCE_PATH "data/textures.fgt")

#define RENDER_OBJECT_MAX_COUNT 64

#define RENDER_OBJECT_QUAD_MAX_COUNT 16

typedef struct RenderObjectQuad {
	int32_t handle;
	RenderObjectQuadType type;
} RenderObjectQuad;

typedef struct RenderObject {
	
	// True if this render object is 'loaded' or in use.
	bool active;
	
	// Array of handles to quads being rendered.
	int32_t quadCount;
	RenderObjectQuad *pQuads;
	
} RenderObject;

AreaRenderState globalAreaRenderState = { };

const Vector4F COLOR_WHITE = { 1.0F, 1.0F, 1.0F, 1.0F };

//AreaRenderState globalAreaRenderState = { };

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
				roomTextureCreateInfo.textureID = newString(256, "roomXS");
				roomTextureCreateInfo.cellExtent.width = 8 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 5 * tile_texel_length;
				break;
			case ROOM_SIZE_S:
				roomTextureCreateInfo.textureID = newString(256, "roomS");
				roomTextureCreateInfo.cellExtent.width = 16 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 10 * tile_texel_length;
				break;
			case ROOM_SIZE_M:
				roomTextureCreateInfo.textureID = newString(256, "roomM");
				roomTextureCreateInfo.cellExtent.width = 24 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 15 * tile_texel_length;
				break;
			case ROOM_SIZE_L:
				roomTextureCreateInfo.textureID = newString(256, "roomL");
				roomTextureCreateInfo.cellExtent.width = 32 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 20 * tile_texel_length;
				break;
		}
		
		textureManagerLoadTexture(roomTextureCreateInfo);
		deleteString(&roomTextureCreateInfo.textureID);
	}
}

void terminateRenderManager(void) {
	terminateVulkanManager();
}

void renderFrame(const float timeDelta) {
	glfwPollEvents();
	
	for (int32_t i = 0; i < RENDER_OBJECT_MAX_COUNT; ++i) {
		renderObjectAnimate2(i);
	}
	
	const Vector4F cameraPosition = areaRenderStateGetCameraPosition(&globalAreaRenderState);
	const ProjectionBounds projectionBounds = areaRenderStateGetProjectionBounds(globalAreaRenderState);
	drawFrame(timeDelta, cameraPosition, projectionBounds);
}

static ModelPool getModelPoolType2(const RenderObjectQuadType quadType);

static RenderObject renderObjects[RENDER_OBJECT_MAX_COUNT];

int32_t loadRenderObject3(const RenderObjectLoadInfo loadInfo) {
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
	if (!validateRenderObjectHandle2(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error loading render object: no render object slots available.");
		return -1;
	}
	
	renderObjects[handle].quadCount = loadInfo.quadCount;
	renderObjects[handle].pQuads = calloc(loadInfo.quadCount, sizeof(RenderObjectQuad));
	if (!renderObjects[handle].pQuads) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error loading render object: failed to allocate quad array.");
		return -1;
	}
	for (int32_t i = 0; i < loadInfo.quadCount; ++i) {
		renderObjects[handle].pQuads[i] = (RenderObjectQuad){
			.handle = -1,
			.type = QUAD_TYPE_MAIN
		};
	}
	
	const Texture texture = getTexture(findTexture(loadInfo.textureID));
	
	for (int32_t quadIndex = 0; quadIndex < loadInfo.quadCount; ++quadIndex) {
		
		const QuadLoadInfo quadLoadInfo = loadInfo.pQuadLoadInfos[quadIndex];
		
		int32_t imageIndex = 0;
		if (quadLoadInfo.initAnimation >= 0 && quadLoadInfo.initAnimation < (int32_t)texture.numAnimations) {
			imageIndex = texture.animations[quadLoadInfo.initAnimation].startCell + quadLoadInfo.initCell;
		}
		
		const ModelLoadInfo modelLoadInfo = {
			.modelPool = getModelPoolType2(quadLoadInfo.quadType),
			.position = vec3DtoVec4F(quadLoadInfo.initPosition),
			.dimensions = quadLoadInfo.quadDimensions,
			.textureID = loadInfo.textureID,
			.color = quadLoadInfo.color,
			.imageIndex = imageIndex
		};
		
		loadModel(modelLoadInfo, &renderObjects[handle].pQuads[quadIndex].handle);
		renderObjects[handle].pQuads[quadIndex].type = quadLoadInfo.quadType;
	}
	
	renderObjects[handle].active = true;
	logMsg(loggerRender, LOG_LEVEL_VERBOSE, "Loaded render object %i.", handle);
	return handle;
}

int32_t loadRenderText(const String text, const Vector3D position, const Vector3D color) {
	
	QuadLoadInfo quadLoadInfos[text.length] = { };
	for (int32_t i = 0; i < (int32_t)text.length; ++i) {
		quadLoadInfos[i] = (QuadLoadInfo){
			.quadType = QUAD_TYPE_MAIN,
			.initPosition = addVec(position, mulVec3D((Vector3D){ 0.5, 0.0, 0.0 }, (double)i)),
			.quadDimensions = (BoxF){
				.x1 = -0.25F,
				.y1 = -0.25F,
				.x2 = 0.25F,
				.y2 = 0.25F
			},
			.initAnimation = 0,
			.initCell = (int32_t)text.pBuffer[i],
			.color = vec3DtoVec4F(color)
		};
	}
	
	const RenderObjectLoadInfo loadInfo = {
		.textureID = (String){
			.length = 17,
			.capacity = 18,
			.pBuffer = "gui/fontFrogBlock"
		},
		.quadCount = text.length,
		.pQuadLoadInfos = quadLoadInfos
	};
	
	return loadRenderObject3(loadInfo);
}

void unloadRenderObject3(int32_t *const pHandle) {
	if (!pHandle) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error unloading render object: pointer to render object handle is null.");
		return;
	} else if (!renderObjectExists(*pHandle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error unloading render object: render object %i does not exist.", *pHandle);
		return;
	}
	
	for (int32_t quadIndex = 0; quadIndex < renderObjects[*pHandle].quadCount; ++quadIndex) {
		if (renderObjects[*pHandle].pQuads[quadIndex].handle >= 0) {
			const ModelPool modelPool = getModelPoolType2(renderObjects[*pHandle].pQuads[quadIndex].type);
			unloadModel(modelPool, &renderObjects[*pHandle].pQuads[quadIndex].handle);
		}
	}
	renderObjects[*pHandle].active = false;
	renderObjects[*pHandle].quadCount = 0;
	free(renderObjects[*pHandle].pQuads);
	
	logMsg(loggerRender, LOG_LEVEL_VERBOSE, "Unloaded render object %i.", *pHandle);
	*pHandle = -1;
}

bool validateRenderObjectHandle2(const int32_t handle) {
	return handle >= 0 && handle < RENDER_OBJECT_MAX_COUNT;
}

bool renderObjectExists(const int32_t handle) {
	return validateRenderObjectHandle2(handle) && renderObjects[handle].active;
}

// TODO: implement function.
int32_t renderObjectLoadQuad2(const int32_t handle, const QuadLoadInfo loadInfo) {
	assert(false);
	(void)handle;
	(void)loadInfo;
	return -1;
}

bool validateRenderObjectQuadIndex2(const int32_t quadIndex) {
	return quadIndex >= 0 && quadIndex < RENDER_OBJECT_QUAD_MAX_COUNT;
}

bool renderObjectQuadExists(const int32_t handle, const int32_t quadIndex) {
	return renderObjectExists(handle) && validateRenderObjectQuadIndex2(quadIndex) && renderObjects[handle].pQuads[quadIndex].handle >= 0;
}

void renderObjectSetPosition2(const int32_t handle, const int32_t quadIndex, const Vector3D position) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error setting render object position: render object %i does not exist.", handle);
		return;
	} else if (!renderObjectQuadExists(handle, quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error setting render object position: quad %i of render object %i does not exist.", quadIndex, handle);
		return;
	}
	
	const ModelPool modelPool = getModelPoolType2(renderObjects[handle].pQuads[quadIndex].type);
	modelSetTranslation(modelPool, renderObjects[handle].pQuads[quadIndex].handle, vec3DtoVec4F(position));
}

int32_t renderObjectGetTextureHandle2(const int32_t handle, const int32_t quadIndex) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error getting render object texture handle: render object %i does not exist.", handle);
		return -1;
	} else if (!renderObjectQuadExists(handle, quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error getting render object texture handle: quad %i of render object %i does not exist.", quadIndex, handle);
		return -1;
	}
	
	const ModelPool modelPool = getModelPoolType2(renderObjects[handle].pQuads[quadIndex].type);
	TextureState *const pTextureState = modelGetTextureState(modelPool, renderObjects[handle].pQuads[quadIndex].handle);
	if (pTextureState) {
		return pTextureState->textureHandle;
	}
	return -1;
}

void renderObjectAnimate2(const int32_t handle) {
	if (!renderObjectExists(handle)) {
		//logMsg(loggerRender, LOG_LEVEL_ERROR, "Error animating render object: render object %i does not exist.", handle);
		return;
	}
	
	for (int32_t quadIndex = 0; quadIndex < renderObjects[handle].quadCount; ++quadIndex) {
		const int32_t quadHandle = renderObjects[handle].pQuads[quadIndex].handle;
		if (quadHandle < 0) {
			continue;
		}
		
		const ModelPool modelPool = getModelPoolType2(renderObjects[handle].pQuads[quadIndex].type);
		TextureState *const pTextureState = modelGetTextureState(modelPool, quadHandle);
		if (textureStateAnimate(pTextureState) == 2) {
			const uint32_t imageIndex = pTextureState->startCell + pTextureState->currentFrame;
			const ModelPool modelPool = getModelPoolType2(renderObjects[handle].pQuads[quadIndex].type);
			updateDrawInfo(modelPool, quadHandle, imageIndex);
		}
	}
}

uint32_t renderObjectGetAnimation2(const int32_t handle, const int32_t quadIndex) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error getting render object animation: render object %i does not exist.", handle);
		return 0;
	} else if (!renderObjectQuadExists(handle, quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error getting render object animation: quad %i of render object %i does not exist.", quadIndex, handle);
		return 0;
	}
	
	const ModelPool modelPool = getModelPoolType2(renderObjects[handle].pQuads[quadIndex].type);
	TextureState *const pTextureState = modelGetTextureState(modelPool, renderObjects[handle].pQuads[quadIndex].handle);
	if (pTextureState) {
		return pTextureState->currentAnimation;
	}
	return 0;
}

bool renderObjectSetAnimation2(const int32_t handle, const int32_t quadIndex, const uint32_t nextAnimation) {
	if (!renderObjectExists(handle)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error setting render object animation: render object %i does not exist.", handle);
		return false;
	} else if (!renderObjectQuadExists(handle, quadIndex)) {
		logMsg(loggerRender, LOG_LEVEL_ERROR, "Error setting render object animation: quad %i of render object %i does not exist.", quadIndex, handle);
		return false;
	}
	
	const ModelPool modelPool = getModelPoolType2(renderObjects[handle].pQuads[quadIndex].type);
	TextureState *const pTextureState = modelGetTextureState(modelPool, renderObjects[handle].pQuads[quadIndex].handle);
	if (pTextureState) {
		if (!textureStateSetAnimation(pTextureState, nextAnimation)) {
			return false;
		}
		const uint32_t imageIndex = pTextureState->startCell + pTextureState->currentFrame;
		updateDrawInfo(modelPool, renderObjects[handle].pQuads[quadIndex].handle, imageIndex);
		return true;
	}
	return false;
}

static ModelPool getModelPoolType2(const RenderObjectQuadType quadType) {
	switch (quadType) {
		default:
		case QUAD_TYPE_MAIN: return modelPoolMain;
		case QUAD_TYPE_DEBUG: return modelPoolDebug;
	};
}