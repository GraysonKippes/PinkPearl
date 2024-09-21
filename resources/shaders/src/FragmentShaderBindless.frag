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
	// Additional draw data
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

layout(location = 0) in vec2 inTextureCoordinates;
layout(location = 1) in vec3 inColor;
layout(location = 2) in flat uint inDrawIndex;

layout(location = 0) out vec4 outColor;

void main() {

	DrawInfo drawInfo = drawInfoBuffers[0].drawInfos[inDrawIndex];
	uint descriptorIndex = drawInfo.modelIndex + pushConstants.descriptorIndexOffset;

	const vec3 textureCoordinates = vec3(inTextureCoordinates, 1.0);
	outColor = texture(sampler2DArray(sampledImages[0], samplers[0]), textureCoordinates) * vec4(inColor, 1.0);
}
