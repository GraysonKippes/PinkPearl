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

AreaRenderState globalAreaRenderState = { };

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
	
	// TEMPORARY
	// Create room textures -- one for each room size.
	for (int i = 0; i < (int)num_room_sizes; ++i) {
		
		// Give each room texture enough layers for each room layer (background, foreground) and each room cache slot.
		TextureCreateInfo roomTextureCreateInfo = (TextureCreateInfo){
			.textureID = makeNullString(),
			.isLoaded = false,
			.isTilemap = false,
			.numCells.width = 1,
			.numCells.length = num_room_layers * num_room_texture_cache_slots,
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
	
	for (int i = 0; i < (int)numRenderObjectSlots; ++i) {
		renderObjectAnimate(i);
	}
	
	const Vector4F cameraPosition = areaRenderStateGetCameraPosition(&globalAreaRenderState);
	const ProjectionBounds projectionBounds = areaRenderStateGetProjectionBounds(globalAreaRenderState);
	drawFrame(deltaTime, cameraPosition, projectionBounds);
}
