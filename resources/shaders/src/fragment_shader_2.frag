#version 450

#define MAX_NUM_MODELS 64

layout(push_constant) uniform draw_data_t {
	uint m_model_slot;	// in the range [0, MAX_NUM_MODELS - 1].
} draw_data;

layout(binding = 1) uniform sampler2D texture_sampler;
layout(binding = 2) uniform sampler2D pbr_sampler;

layout(location = 0) in vec2 frag_tex_coord;
layout(location = 1) in vec3 frag_position;	// Model space

layout(location = 0) out vec4 out_color;

float calculate_attenuation(vec3 src, vec3 dst, float k_q, float k_l) {
	float d = distance(src, dst);
	float r = k_q * (d * d) + k_l * d + 1.0;
	return 1.0 / r;
}

void main() {

	vec4 pbr = texture(pbr_sampler, frag_tex_coord);

	vec4 ambient_lighting_color = vec4(0.5, 0.75, 1.0, 1.0);
	float ambient_lighting_intensity = 1.0;

	// Ambient light
	vec3 point_light_position = vec3(0.0, -3.5, 0.0);
	vec4 point_light_color = vec4(0.0, 0.5, 1.0, 1.0);
	float point_light_intensity = 1.0;

	// Texel position
	vec3 texel_position = frag_position;
	texel_position.x = floor(frag_position.x * 16.0) / 16.0;
	texel_position.y = floor(frag_position.y * 16.0) / 16.0;

	// Height map
	float height = pbr.z * 32.0;
	point_light_position.z -= height;

	// Point light
	out_color = texture(texture_sampler, frag_tex_coord);
	out_color *= ambient_lighting_color * ambient_lighting_intensity;
	out_color *= point_light_color * point_light_intensity * calculate_attenuation(point_light_position, texel_position, 0.44, 0.35);
	
	// Light level granularity
	const float light_level = 128.0;
	out_color.x = round(out_color.x * light_level) / light_level;
	out_color.y = round(out_color.y * light_level) / light_level;
	out_color.z = round(out_color.z * light_level) / light_level;

	// PBR
	out_color.xyz *= pbr.x;
}
