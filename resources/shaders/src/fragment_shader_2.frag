#version 450

#define MAX_NUM_MODELS 64

layout(push_constant) uniform draw_data_t {
	uint m_model_slot;	// in the range [0, MAX_NUM_MODELS - 1].
} draw_data;

layout(binding = 1) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 frag_tex_coord;
layout(location = 1) in vec3 frag_position;	// Model space

layout(location = 0) out vec4 out_color;

float calculate_attenuation(vec3 src, vec3 dst) {
	float d = distance(src, dst);
	float r = 0.44 * (d * d) + 0.35 * d + 1.0;
	return 1.0 / r;
}

void main() {

	vec4 ambient_lighting_color = vec4(0.5, 0.5, 1.0, 1.0);
	float ambient_lighting_intensity = 0.75F;

	// Ambient light
	vec3 point_light_position = vec3(-2.0, 3.0, 0.0);
	vec4 point_light_color = vec4(0.0, 0.5, 1.0, 1.0);
	float point_light_intensity = 1.0;

	// Texel position
	vec3 texel_position = frag_position;
	texel_position.x = floor(frag_position.x * 16.0) / 16.0;
	texel_position.y = floor(frag_position.y * 16.0) / 16.0;

	// Point light
	out_color = texture(texture_sampler, frag_tex_coord);
	out_color *= ambient_lighting_color * ambient_lighting_intensity;
	out_color *= point_light_color * point_light_intensity * calculate_attenuation(point_light_position, texel_position);
}
