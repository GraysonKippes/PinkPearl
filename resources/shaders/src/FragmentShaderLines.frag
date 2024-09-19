#version 460
#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(inColor, 1.0);
}
