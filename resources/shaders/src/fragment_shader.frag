#version 460
#extension GL_EXT_scalar_block_layout : require

#define NUM_RENDER_OBJECTS 64
#define NUM_ROOM_TEXTURE_IMAGES 4
#define NUM_IMAGES (NUM_RENDER_OBJECTS + NUM_ROOM_TEXTURE_IMAGES)

struct draw_info_t {
	// Indirect draw info
	uint index_count;
	uint instance_count;
	uint first_index;
	int vertex_offset;
	uint first_instance;
	// Additional draw data
	uint render_object_slot;
	uint image_index;
};

layout(scalar, set = 0, binding = 0) readonly uniform draw_data_t {
	draw_info_t draw_infos[68];
} draw_data;

layout(set = 0, binding = 2) uniform sampler2DArray[NUM_IMAGES] texture_samplers;

layout(location = 0) in vec3 in_position;	// Model space
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_color;
layout(location = 3) in flat uint in_draw_index;

layout(location = 0) out vec4 out_color;

float calculate_attenuation(vec3 src, vec3 dst, float k_q, float k_l) {
	float d = distance(src, dst);
	float r = k_q * (d * d) + k_l * d + 1.0;
	return 1.0 / r;
}

void main() {

	draw_info_t draw_info = draw_data.draw_infos[in_draw_index];

	// Ambient light
	const vec3 ambient_light_color = vec3(0.125, 0.25, 0.5);
	const float ambient_lighting_intensity = 0.825;
	const vec3 ambient_lighting = ambient_light_color * ambient_lighting_intensity;

	// Texel position
	vec3 texel_position = in_position;
	texel_position.x = floor(in_position.x * 16.0) / 16.0;
	texel_position.y = floor(in_position.y * 16.0) / 16.0;

	// Apply lighting
	const vec3 texture_coordinates = vec3(in_tex_coord, float(draw_info.image_index)) * in_color;
	out_color = texture(texture_samplers[draw_info.render_object_slot], texture_coordinates);
	out_color.xyz *= ambient_lighting;
	
	// Light level granularity
	const float light_level = 32.0;
	out_color.x = round(out_color.x * light_level) / light_level;
	out_color.y = round(out_color.y * light_level) / light_level;
	out_color.z = round(out_color.z * light_level) / light_level;
}
