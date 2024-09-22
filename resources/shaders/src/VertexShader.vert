#version 460
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

#define MAX_MODEL_COUNT 512

// Type/struct definitions

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

// Shader layout defintions

layout(set = 0, binding = 0) uniform sampler samplers[];

layout(set = 0, binding = 1) uniform texture2DArray sampledImages[];

layout(set = 0, binding = 2, rgba8ui) uniform uimage2DArray storageImages[];

// TODO: use buffer descriptor aliasing for various buffer configurations (e.g. matrices, lighting data).

layout(set = 0, binding = 3, scalar) uniform DrawInfoBuffers {
	uint drawCount;
	DrawInfo drawInfos[MAX_MODEL_COUNT];
} drawInfoBuffers[];

layout(set = 0, binding = 4, scalar) buffer MatrixBuffers {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 modelMatrices[MAX_MODEL_COUNT];
} matrixBuffers[];

layout(push_constant) uniform PushConstants {
	uint samplerIndex;
	uint sampledImageIndex;
	uint storageImageIndex;
	uint uniformBufferIndex;
	uint storageBufferIndex;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextureCoordinates;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec2 outTextureCoordinates;
layout(location = 1) out vec3 outColor;
layout(location = 2) out uint outDrawIndex;

// Function definitions

void main() {
	DrawInfo drawInfo = drawInfoBuffers[pushConstants.uniformBufferIndex].drawInfos[gl_DrawID];
	
	mat4 modelMatrix = matrixBuffers[pushConstants.storageBufferIndex].modelMatrices[drawInfo.modelIndex];
	vec4 homogenousCoordinates = vec4(inPosition, 1.0);
	gl_Position = matrixBuffers[pushConstants.storageBufferIndex].projectionMatrix * matrixBuffers[pushConstants.storageBufferIndex].viewMatrix * modelMatrix * homogenousCoordinates;

	outTextureCoordinates = inTextureCoordinates;
	outColor = inColor;
	outDrawIndex = gl_DrawID;
}
