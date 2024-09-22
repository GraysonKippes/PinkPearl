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
	// Additional draw info
	int modelIndex;
	uint imageIndex;
};

layout(set = 0, binding = 0) uniform sampler samplers[];

layout(set = 0, binding = 1) uniform texture2DArray sampledImages[];

layout(set = 0, binding = 2, rgba8ui) uniform uimage2DArray storageImages[];

// TODO: use buffer descriptor aliasing for various buffer configurations (e.g. matrices, lighting data).

layout(set = 0, binding = 3, scalar) uniform U {
	uint drawCount;
	DrawInfo drawInfos[MAX_MODEL_COUNT];
} drawInfoBuffers[];

layout(set = 0, binding = 4) buffer S {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 modelMatrices[MAX_MODEL_COUNT];
} matrixBuffers[];

// TODO: take out descriptorIndexOffset, add in a descriptor index push constant for each descriptor binding
layout(push_constant) uniform PushConstants {
	uint descriptorIndexOffset;
	uint descriptorIndex;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;

void main() {
	DrawInfo drawInfo = drawInfoBuffers[pushConstants.descriptorIndex].drawInfos[gl_DrawID];
	uint descriptorIndex = pushConstants.descriptorIndexOffset + drawInfo.modelIndex;	// get rid of this
	
	mat4 modelMatrix = matrixBuffers[pushConstants.descriptorIndex].modelMatrices[drawInfo.modelIndex];
	vec4 homogenousCoordinates = vec4(inPosition, 1.0);
	gl_Position = matrixBuffers[pushConstants.descriptorIndex].projectionMatrix * matrixBuffers[pushConstants.descriptorIndex].viewMatrix * modelMatrix * homogenousCoordinates;

	outColor = inColor;
}
