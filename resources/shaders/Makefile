SLC = glslc

shaders: compute_matrices vertex_models fragment_shader

compute_matrices: src/compute_matrices.comp
	$(SLC) $< -o compute_matrices.spv

compute_textures: src/compute_textures.comp
	$(SLC) $< -o compute_textures.spv

vertex_shader: src/vertex_shader.vert
	$(SLC) $< -o vertex_shader.spv

vertex_models: src/vertex_models.vert
	$(SLC) $< -o vertex_models.spv

fragment_shader: src/fragment_shader.frag
	$(SLC) $< -o fragment_shader.spv

fragment_test: src/fragment_test.frag
	$(SLC) $< -o fragment_test.spv
