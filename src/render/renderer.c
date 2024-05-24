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
	create_vulkan_render_objects();
	init_render_object_manager();

	texture_pack_t texture_pack = parse_fgt_file(FGT_PATH);
	load_textures(texture_pack);
	destroy_texture_pack(&texture_pack);
	
	log_stack_pop();
}

void terminate_renderer(void) {
	log_stack_push("Renderer");
	destroy_vulkan_render_objects();
	destroy_vulkan_objects();
	log_stack_pop();
}

void render_frame(const float deltaTime) {
	
	const vector4F_t cameraPosition = areaRenderStateGetCameraPosition(&globalAreaRenderState);
	const projection_bounds_t projectionBounds = areaRenderStateGetProjectionBounds(globalAreaRenderState);
	
	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		if (textureStateAnimate(getRenderObjTexState((int)i)) == 2) {
			// TODO - replace with update draw data
			//upload_draw_data(globalAreaRenderState);
		}
	}

	glfwPollEvents();
	drawFrame(deltaTime, cameraPosition, projectionBounds, getRenderObjTransform(0));
}
