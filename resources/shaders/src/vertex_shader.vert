#version 450

#define NUM_MODELS 64

layout(push_constant) uniform draw_data_t {
	uint model_slot;
} draw_data;

layout(binding = 0) readonly buffer matrix_buffer_t {
	mat4 camera_matrix;
	mat4 projection_matrix;
	mat4 matrices[NUM_MODELS];
} matrix_buffer;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_color;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec2 out_tex_coord;
layout(location = 2) out vec3 out_color;

const mat4 invert_y_matrix = mat4(
	1.0, 0.0, 0.0, 0.0,
	0.0, -1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
);

void main() {

	vec4 homogenous_coordinates = vec4(in_position, 1.0);

	mat4 model_matrix = mat4(1.0);
	if (draw_data.model_slot < NUM_MODELS) {
		model_matrix = matrix_buffer.matrices[draw_data.model_slot];
	}

	gl_Position = matrix_buffer.projection_matrix * matrix_buffer.camera_matrix * model_matrix * homogenous_coordinates;

	out_position = vec3(model_matrix * homogenous_coordinates);
	out_tex_coord = in_tex_coord;
	out_color = in_color;
}
