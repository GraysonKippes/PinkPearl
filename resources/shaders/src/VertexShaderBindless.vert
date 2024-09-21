#version 460
#extension GL_EXT_scalar_block_layout : require

#define MAX_MODEL_COUNT 512

struct DrawInfo {
	// Indirect draw info
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
	// Additional draw info
	int modelIndex;
	uint imageIndex;
};

layout(set = 0, binding = 0) uniform sampler samplers[];

layout(set = 0, binding = 1) uniform texture2DArray sampledImages[];

layout(set = 0, binding = 2, rgba8ui) uniform uimage2DArray storageImages[];

layout(set = 0, binding = 3) uniform U {
	uint drawCount;
	DrawInfo drawInfos[MAX_MODEL_COUNT];
} drawInfoBuffers[];

layout(set = 0, binding = 4) buffer S {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 modelMatrices[MAX_MODEL_COUNT];
} matrixBuffers[];

layout(push_constant) uniform PushConstants {
	uint descriptorIndexOffset;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextureCoordinates;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec2 outTextureCoordinates;
layout(location = 1) out vec3 outColor;
layout(location = 2) out uint outDrawIndex;

void main() {

	DrawInfo drawInfo = drawInfoBuffers[0].drawInfos[gl_DrawID];
	uint descriptorIndex = drawInfo.modelIndex + pushConstants.descriptorIndexOffset;
	
	mat4 modelMatrix = matrixBuffers[0].modelMatrices[descriptorIndex];
	vec4 homogenousCoordinates = vec4(inPosition, 1.0);
	gl_Position = matrixBuffers[0].projectionMatrix * matrixBuffers[0].viewMatrix * modelMatrix * homogenousCoordinates;

	outTextureCoordinates = inTextureCoordinates;
	outColor = inColor;
	outDrawIndex = gl_DrawID;
}
