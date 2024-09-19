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

layout(push_constant) uniform PushConstants {
	uint descriptorIndexOffset;
} pushConstants;

layout(scalar, set = 0, binding = 0) readonly uniform UDrawData {
	uint drawCount;
	DrawInfo drawInfos[MAX_MODEL_COUNT];
} uDrawData;

layout(set = 0, binding = 1) readonly buffer MatrixBuffer {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 modelMatrices[MAX_MODEL_COUNT];
} matrixBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;

void main() {

	DrawInfo drawInfo = uDrawData.drawInfos[gl_DrawID];
	uint descriptorIndex = drawInfo.modelIndex + pushConstants.descriptorIndexOffset;
	
	mat4 modelMatrix = matrixBuffer.modelMatrices[descriptorIndex];

	vec4 homogenous_coordinates = vec4(inPosition, 1.0);
	gl_Position = matrixBuffer.projectionMatrix * matrixBuffer.viewMatrix * modelMatrix * homogenous_coordinates;

	outColor = inColor;
}
