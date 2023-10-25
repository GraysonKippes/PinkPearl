#version 450

#define MAX_NUM_MODELS 64

layout(push_constant) uniform draw_data_t {
	uint m_model_slot;	// in the range [0, MAX_NUM_MODELS - 1].
} draw_data;

layout(set = 0, binding = 0) readonly buffer matrix_buffer_t {
	mat4 m_matrices[MAX_NUM_MODELS];
} matrix_buffer;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_color;

layout(location = 0) out vec2 frag_tex_coord;
layout(location = 1) out vec3 frag_color;

void main() {
	gl_Position = matrix_buffer.m_matrices[draw_data.m_model_slot] * vec4(in_position, 1.0);
	frag_tex_coord = in_tex_coord;
	frag_color = in_color;
}
