#version 450

#define NUM_RENDER_OBJECTS 64
#define NUM_ROOM_TEXTURE_IMAGES 4
#define NUM_IMAGES (NUM_RENDER_OBJECTS + NUM_ROOM_TEXTURE_IMAGES)

layout(push_constant) uniform draw_data_t {
	uint model_slot;
	uint animation_frame;
} draw_data;

layout(binding = 1) uniform sampler2DArray[NUM_IMAGES] texture_samplers;

layout(location = 0) in vec3 in_position;	// Model space
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_color;

layout(location = 0) out vec4 out_color;

float calculate_attenuation(vec3 src, vec3 dst, float k_q, float k_l) {
	float d = distance(src, dst);
	float r = k_q * (d * d) + k_l * d + 1.0;
	return 1.0 / r;
}

void main() {

	const vec3 texture_coordinates = vec3(in_tex_coord, float(draw_data.animation_frame)) * in_color;

	// Ambient light
	const vec3 ambient_light_color = vec3(1.0, 0.75, 0.5);
	const float ambient_lighting_intensity = 0.825;
	const vec3 ambient_lighting = ambient_light_color * ambient_lighting_intensity;

	// Texel position
	vec3 texel_position = in_position;
	texel_position.x = floor(in_position.x * 16.0) / 16.0;
	texel_position.y = floor(in_position.y * 16.0) / 16.0;

	// Apply lighting
	out_color = texture(texture_samplers[draw_data.model_slot], texture_coordinates);
	out_color.xyz *= ambient_lighting;
	
	// Light level granularity
	const float light_level = 32.0;
	out_color.x = round(out_color.x * light_level) / light_level;
	out_color.y = round(out_color.y * light_level) / light_level;
	out_color.z = round(out_color.z * light_level) / light_level;
}
