#include "renderer.h"

#include <stdbool.h>

#include <GLFW/glfw3.h>

#include "config.h"
#include "log/log_stack.h"
#include "log/logging.h"

#include "area_render_state.h"
#include "render_config.h"
#include "render_object.h"
#include "vulkan/texture_manager.h"
#include "vulkan/vulkan_manager.h"
#include "vulkan/vulkan_render.h"
#include "vulkan/math/projection.h"
#include "vulkan/math/vector4F.h"

#define DATA_PATH (RESOURCE_PATH "data/")
#define FGT_PATH (RESOURCE_PATH "data/textures.fgt")

AreaRenderState globalAreaRenderState = { 0 };

void init_renderer(void) {
	log_stack_push("Renderer");

	create_vulkan_objects();
	createVulkanRenderObjects();
	initRenderObjectManager();
	initTextureManager();
	initTextureDescriptors();

	TexturePack texturePack = readTexturePackFile(FGT_PATH);
	textureManagerLoadTexturePack(texturePack);
	deleteTexturePack(&texturePack);
	
	for (int i = 0; i < (int)num_room_sizes; ++i) {
		
		TextureCreateInfo roomTextureCreateInfo = (TextureCreateInfo){
			.textureID = makeNullString(),
			.isLoaded = false,
			.isTilemap = false,
			.numCells.width = 1,
			.numCells.length = num_room_layers,
			.cellExtent.width = 16,
			.cellExtent.length = 16,
			.numAnimations = 1,
			.animations = (TextureAnimation[1]){
				{
					.startCell = 0,
					.numFrames = 1,
					.framesPerSecond = 0
				}
			}
		};
		
		switch((RoomSize)i) {
			case ONE_TO_THREE:
				roomTextureCreateInfo.textureID = newString(256, "roomXS");
				roomTextureCreateInfo.cellExtent.width = 8 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 5 * tile_texel_length;
				break;
			case TWO_TO_THREE:
				roomTextureCreateInfo.textureID = newString(256, "roomS");
				roomTextureCreateInfo.cellExtent.width = 16 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 10 * tile_texel_length;
				break;
			case THREE_TO_THREE:
				roomTextureCreateInfo.textureID = newString(256, "roomM");
				roomTextureCreateInfo.cellExtent.width = 24 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 15 * tile_texel_length;
				break;
			case FOUR_TO_THREE:
				roomTextureCreateInfo.textureID = newString(256, "roomL");
				roomTextureCreateInfo.cellExtent.width = 32 * tile_texel_length;
				roomTextureCreateInfo.cellExtent.length = 20 * tile_texel_length;
				break;
		}
		
		textureManagerLoadTexture(roomTextureCreateInfo);
		deleteString(&roomTextureCreateInfo.textureID);
	}
	
	
	log_stack_pop();
}

void terminate_renderer(void) {
	log_stack_push("Renderer");
	destroyVulkanRenderObjects();
	destroy_vulkan_objects();
	terminateRenderObjectManager();
	log_stack_pop();
}

void render_frame(const float deltaTime) {
	glfwPollEvents();
	
	for (int i = 0; i < (int)num_render_object_slots; ++i) {
		TextureState *pTextureState = getRenderObjTexState(i);
		if (textureStateAnimate(pTextureState) == 2) {
			const unsigned int imageIndex = pTextureState->startCell + pTextureState->currentFrame;
			updateDrawData(i, imageIndex);
		}
	}
	
	const Vector4F cameraPosition = areaRenderStateGetCameraPosition(&globalAreaRenderState);
	const projection_bounds_t projectionBounds = areaRenderStateGetProjectionBounds(globalAreaRenderState);

	drawFrame(deltaTime, cameraPosition, projectionBounds, getRenderObjTransform(0));
}
