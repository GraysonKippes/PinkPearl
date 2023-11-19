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

	vec3 texture_coordinates = vec3(frag_tex_coord, float(draw_data.animation_frame));

	vec4 pbr = texture(pbr_sampler, texture_coordinates);

	vec3 global_light_color = vec3(0.5, 0.75, 1.0);

	// Ambient light
	float ambient_lighting_intensity = 0.825;
	vec3 ambient_lighting = global_light_color * ambient_lighting_intensity;

	// Diffuse light
	float diffuse_lighting_intensity = 0.25;
	vec3 surface_direction = normalize(vec3(0.0, pbr.g, 1.0 - pbr.g));
	vec3 diffuse_direction = normalize(vec3(0.0, 1.0, 1.0));
	float diffuse = max(dot(surface_direction, diffuse_direction), 0.0);
	vec3 diffuse_lighting = global_light_color * diffuse_lighting_intensity * diffuse;

	// Point light
	vec3 point_light_position = vec3(0.0, -0.5, 1.5);
	vec4 point_light_color = vec4(1.0, 0.5, 0.0, 1.0);
	float point_light_intensity = 1.0;

	// Texel position
	vec3 texel_position = frag_position;
	texel_position.x = floor(frag_position.x * 16.0) / 16.0;
	texel_position.y = floor(frag_position.y * 16.0) / 16.0;

	// Height map
	float height_factor = 4.0;
	if (pbr.z > texel_position.z) {
		vec3 next_tex_coord = texture_coordinates;
		vec4 next = pbr;
		while (next.z > texel_position.z) {
			next_tex_coord.y -= 0.0625;
			next = texture(pbr_sampler, next_tex_coord);
		}
		texel_position.y = next_tex_coord.y;
	}
	texel_position.z += pbr.z * height_factor;

	// Apply lighting
	out_color = texture(texture_samplers[draw_data.model_slot], texture_coordinates);

	return;

	out_color.xyz *= (ambient_lighting + diffuse_lighting);

	out_color *= point_light_color * point_light_intensity 
			* calculate_attenuation(point_light_position, texel_position, 0.07, 0.14);
	
	// Light level granularity
	const float light_level = 32.0;
	out_color.x = round(out_color.x * light_level) / light_level;
	out_color.y = round(out_color.y * light_level) / light_level;
	out_color.z = round(out_color.z * light_level) / light_level;

	// PBR
	out_color.xyz *= pbr.x;
}
