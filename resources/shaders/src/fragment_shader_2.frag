#version 450

#define MAX_NUM_MODELS 64

layout(push_constant) uniform draw_data_t {
	uint m_model_slot;	// in the range [0, MAX_NUM_MODELS - 1].
} draw_data;

layout(binding = 1) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

void main() {
	out_color = texture(texture_sampler, frag_tex_coord);
}
