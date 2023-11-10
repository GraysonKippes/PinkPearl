SLC = glslc

shaders: compute_matrices compute_textures room_texture vertex_models fragment_shader_2

compute_matrices: src/compute_matrices.comp
	$(SLC) $< -o compute_matrices.spv

compute_textures: src/compute_textures.comp
	$(SLC) $< -o compute_textures.spv

room_texture: src/room_texture.comp
	$(SLC) $< -o room_texture.spv

vertex_shader: src/vertex_shader.vert
	$(SLC) $< -o vertex_shader.spv

vertex_models: src/vertex_models.vert
	$(SLC) $< -o vertex_models.spv

fragment_shader: src/fragment_shader.frag
	$(SLC) $< -o fragment_shader.spv

fragment_shader_2: src/fragment_shader_2.frag
	$(SLC) $< -o fragment_shader_2.spv
