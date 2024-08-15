#version 460
#extension GL_EXT_scalar_block_layout : require

#define NUM_QUADS 256

struct DrawInfo {
	// Indirect draw info
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
	// Additional draw data
	int quadID;
	uint imageIndex;
};

layout(scalar, set = 0, binding = 0) readonly uniform DrawData {
	DrawInfo draw_infos[NUM_QUADS];
} drawData;

layout(set = 0, binding = 1) readonly buffer MatrixBuffer {
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 modelMatrices[NUM_QUADS];
} matrixBuffer;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_color;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec2 out_tex_coord;
layout(location = 2) out vec3 out_color;
layout(location = 3) out uint out_draw_index;

void main() {

	out_draw_index = gl_DrawID;
	DrawInfo draw_info = drawData.draw_infos[gl_DrawID];

	mat4 modelMatrix = mat4(1.0);
	modelMatrix = matrixBuffer.modelMatrices[draw_info.quadID];

	vec4 homogenous_coordinates = vec4(in_position, 1.0);
	gl_Position = matrixBuffer.projectionMatrix * matrixBuffer.viewMatrix * modelMatrix * homogenous_coordinates;

	out_position = vec3(modelMatrix * homogenous_coordinates);
	out_tex_coord = in_tex_coord;
	out_color = in_color;
}
