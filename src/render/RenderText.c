#include "RenderText.h"

#include <stdlib.h>

#include "vulkan/Draw.h"
#include "vulkan/VulkanManager.h"

struct RenderTextObject_T {
	
	String text;
	
	int *pQuadHandles;
	
};

RenderTextObject loadRenderTextObject(const String text, const Vector3D position, const Vector3D color) {
	
	RenderTextObject renderTextObject = malloc(sizeof(struct RenderTextObject_T));
	if (!renderTextObject) {
		return nullptr;
	}
	
	renderTextObject->pQuadHandles = malloc(text.capacity * sizeof(int));
	if (!renderTextObject->pQuadHandles) {
		free(renderTextObject);
		return nullptr;
	}
	
	for (size_t i = 0; i < text.capacity; ++i) {
		
		const ModelLoadInfo modelLoadInfo = {
			.modelPool = modelPoolMain,
			//.position = vec3DtoVec4F(position),
			.position = (Vector4F){
				.x = position.x + (float)i * 0.5F,
				.y = position.y,
				.z = position.z,
				.w = 1.0F
			},
			.dimensions = (BoxF){
				.x1 = -0.25F,
				.y1 = -0.25F,
				.x2 = 0.25F,
				.y2 = 0.25F
			},
			.textureID = (String){
				.length = 17,
				.capacity = 18,
				.pBuffer = "gui/fontFrogBlock"
			},
			.color = vec3DtoVec4F(color),
			.imageIndex = (uint32_t)text.pBuffer[i]
		};
		
		loadModel(modelLoadInfo, &renderTextObject->pQuadHandles[i]);
	}
	
	return renderTextObject;
}

void unloadRenderTextObject(RenderTextObject *const pRenderTextObject) {
	if (!pRenderTextObject) {
		return;
	}
	
	
}