#version 450

#define NUM_MODELS 64

layout(push_constant) uniform draw_data_t {
	uint model_slot;	// in the range [0, NUM_MODELS - 1].
	uint animation_frame;
} draw_data;

// Base texture
// Rotation map - used in diffuse
// PBR map ()

layout(binding = 1) uniform sampler2DArray[NUM_MODELS] texture_samplers;
layout(binding = 2) uniform sampler2DArray pbr_sampler;

layout(location = 0) in vec2 frag_tex_coord;
layout(location = 1) in vec3 frag_position;	// Model space

layout(location = 0) out vec4 out_color;

float calculate_attenuation(vec3 src, vec3 dst, float k_q, float k_l) {
	float d = distance(src, dst);
	float r = k_q * (d * d) + k_l * d + 1.0;
	return 1.0 / r;
}

void main() {

	const vec3 texture_coordinates = vec3(frag_tex_coord, float(draw_data.animation_frame));

	// Ambient light
	const vec3 ambient_light_color = vec3(0.5, 0.75, 1.0);
	const float ambient_lighting_intensity = 0.825;
	const vec3 ambient_lighting = ambient_light_color * ambient_lighting_intensity;

	// Texel position
	vec3 texel_position = frag_position;
	texel_position.x = floor(frag_position.x * 16.0) / 16.0;
	texel_position.y = floor(frag_position.y * 16.0) / 16.0;

	// Apply lighting
	out_color = texture(texture_samplers[draw_data.model_slot], texture_coordinates);
	out_color.xyz *= ambient_lighting;
	
	// Light level granularity
	const float light_level = 32.0;
	out_color.x = round(out_color.x * light_level) / light_level;
	out_color.y = round(out_color.y * light_level) / light_level;
	out_color.z = round(out_color.z * light_level) / light_level;
}
