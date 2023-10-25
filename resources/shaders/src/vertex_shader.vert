#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragColor;

void main() {

	mat3 test_positions;
	test_positions[0] = vec3(0.0, 1.0, 0.0);
	test_positions[1] = vec3(1.0, -1.0, 0.0);
	test_positions[2] = vec3(-1.0, -1.0, 0.0);

	//gl_Position = vec4(test_positions[gl_VertexIndex], 1.0);

	gl_Position = vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
	fragColor = inColor;
}
