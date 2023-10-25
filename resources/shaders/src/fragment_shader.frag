#version 450

layout(binding = 1) uniform sampler2D texture_sampler;

// Texture coordinates and vertex colors from the vertex shader.
layout(location = 0) in vec2 frag_tex_coord;
layout(location = 1) in vec3 frag_color;

// Output color.
layout(location = 0) out vec4 out_color;

// Types of lighting:
//	Ambient lighting
//	Point-distance lighting

// Pixelated lighting procedure
//	1. get position of texel being sampled
//	2. calculate distance between the point lights and that position, instead of the position of the fragment
//	3. calculate attenuation based on that distance

// Lighting uniform buffer.
layout(binding = 2) uniform lighting_t {

	vec3 ambient;
	float ambient_strength;

} lighting;

void main() {

	vec3 ambient_lighting = lighting.ambient_strength * lighting.ambient;

	out_color = texture(texture_sampler, frag_tex_coord);
	out_color.rgb *= frag_color * ambient_lighting;
}
