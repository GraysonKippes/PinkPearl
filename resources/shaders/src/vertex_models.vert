#version 450

#define NUM_MODELS 64

layout(push_constant) uniform draw_data_t {
	uint model_slot;	// in the range [0, NUM_MODELS - 1].
} draw_data;

layout(binding = 0) readonly buffer matrix_buffer_t {
	mat4 matrices[NUM_MODELS];
} matrix_buffer;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec2 frag_tex_coord;
layout(location = 1) out vec3 frag_position;

void main() {

	gl_Position = matrix_buffer.matrices[1] * matrix_buffer.matrices[0] * matrix_buffer.matrices[draw_data.model_slot + 2] * vec4(in_position, 1.0);

	frag_tex_coord = in_tex_coord;
	frag_position = vec3(matrix_buffer.matrices[draw_data.model_slot + 2] * vec4(in_position, 1.0));
}
