#version 450

// Texture coordinates and vertex colors from the vertex shader.
layout(location = 0) in vec2 frag_tex_coord;
layout(location = 1) in vec3 frag_color;

// Output color.
layout(location = 0) out vec4 out_color;

void main() {
	out_color.rgb = frag_color;
	out_color.a = 1.0;
	//out_color.rgb = vec3(1.0, 1.0, 1.0);
}
