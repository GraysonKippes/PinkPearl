#version 460
#extension GL_EXT_scalar_block_layout : require

#define NUM_RENDER_OBJECTS 256

struct DrawData {
	// Indirect draw info
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
	// Additional draw data
	int quadID;
	uint imageIndex;
};

layout(scalar, set = 0, binding = 0) readonly uniform UDrawData {
	DrawData draw_infos[NUM_RENDER_OBJECTS];
} uDrawData;

layout(set = 0, binding = 2) uniform texture2DArray[NUM_RENDER_OBJECTS] textures;
layout(set = 0, binding = 3) uniform sampler textureSampler;

struct ambient_light_t {
	vec3 color;
	float intensity;
};

struct point_light_t {
	vec3 position;
	vec3 color;
	float intensity;
};

layout(scalar, set = 0, binding = 4) readonly uniform lighting_data_t {

	ambient_light_t ambient_lighting;

	uint num_point_lights;
	point_light_t point_lights[NUM_RENDER_OBJECTS];
	
} lighting_data;

layout(location = 0) in vec3 in_position;	// Model space
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_color;
layout(location = 3) in flat uint in_draw_index;

layout(location = 0) out vec4 out_color;

float calculate_attenuation(const vec3 src, const vec3 dst, const float coefficient_quadratic, const float coefficient_linear) {
	const float d = distance(src, dst);
	const float r = coefficient_quadratic * (d * d) + coefficient_linear * d + 1.0;
	return 1.0 / r;
}

void main() {

	DrawData draw_info = uDrawData.draw_infos[in_draw_index];

	// Texel position
	vec3 texel_position = in_position;
	texel_position.x = floor(in_position.x * 16.0) / 16.0;
	texel_position.y = floor(in_position.y * 16.0) / 16.0;

	const vec3 texture_coordinates = vec3(in_tex_coord, float(draw_info.imageIndex));
	out_color = texture(sampler2DArray(textures[draw_info.quadID], textureSampler), texture_coordinates) * vec4(in_color, 1.0);
	
	/* Apply lighting
	out_color.rgb *= (lighting_data.ambient_lighting.color * lighting_data.ambient_lighting.intensity);
	vec3 point_light_color = vec3(1.0);
	for (uint i = 0; i < lighting_data.num_point_lights; ++i) {
		const point_light_t point_light = lighting_data.point_lights[i];
		point_light_color *= (point_light.color * point_light.intensity * calculate_attenuation(point_light.position, texel_position, 0.07, 0.14));
	}
	out_color.rgb *= point_light_color; */
	
	/* Light level granularity
	const float light_level = 32.0;
	out_color.r = round(out_color.r * light_level) / light_level;
	out_color.g = round(out_color.g * light_level) / light_level;
	out_color.b = round(out_color.b * light_level) / light_level; */
}
