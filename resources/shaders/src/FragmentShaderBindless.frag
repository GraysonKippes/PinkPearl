#version 460
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

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

layout(set = 0, binding = 3, scalar) uniform U {
	uint drawCount;
	DrawInfo drawInfos[MAX_MODEL_COUNT];
} drawInfoBuffers[];

layout(set = 0, binding = 4, scalar) buffer S {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 modelMatrices[MAX_MODEL_COUNT];
} matrixBuffers[];

layout(push_constant) uniform PushConstants {
	uint descriptorIndexOffset;
	uint descriptorIndex;
} pushConstants;

layout(location = 0) in vec2 inTextureCoordinates;
layout(location = 1) in vec3 inColor;
layout(location = 2) in flat uint inDrawIndex;

layout(location = 0) out vec4 outColor;

void main() {
	DrawInfo drawInfo = drawInfoBuffers[pushConstants.descriptorIndex].drawInfos[inDrawIndex];
	uint descriptorIndex = pushConstants.descriptorIndexOffset + drawInfo.modelIndex;	// get rid of this
	
	const vec3 textureCoordinates = vec3(inTextureCoordinates, float(drawInfo.imageIndex));
	outColor = texture(sampler2DArray(sampledImages[descriptorIndex], samplers[0]), textureCoordinates) * vec4(inColor, 1.0);
}
