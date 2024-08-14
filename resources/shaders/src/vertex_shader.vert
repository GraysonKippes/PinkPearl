#version 460
#extension GL_EXT_scalar_block_layout : require

#define NUM_MODELS 256

struct draw_info_t {
	// Indirect draw info
	uint index_count;
	uint instance_count;
	uint first_index;
	int vertex_offset;
	uint first_instance;
	// Additional draw data
	int render_object_slot;
	uint image_index;
};

layout(scalar, set = 0, binding = 0) readonly uniform draw_data_t {
	draw_info_t draw_infos[NUM_MODELS];
} draw_data;

layout(set = 0, binding = 1) readonly buffer matrix_buffer_t {
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
layout(location = 3) out uint out_draw_index;

void main() {

	out_draw_index = gl_DrawID;
	draw_info_t draw_info = draw_data.draw_infos[gl_DrawID];

	mat4 model_matrix = mat4(1.0);
	if (draw_info.render_object_slot < NUM_MODELS) {
		model_matrix = matrix_buffer.matrices[draw_info.render_object_slot];
	}

	vec4 homogenous_coordinates = vec4(in_position, 1.0);
	gl_Position = matrix_buffer.projection_matrix * matrix_buffer.camera_matrix * model_matrix * homogenous_coordinates;

	out_position = vec3(model_matrix * homogenous_coordinates);
	out_tex_coord = in_tex_coord;
	out_color = in_color;
}
