SLC = glslc

shaders: compute_matrices room_texture vertex_shader fragment_shader area_mesh

compute_matrices: src/compute_matrices.comp
	$(SLC) $< -o compute_matrices.spv

room_texture: src/room_texture.comp
	$(SLC) $< -o room_texture.spv

vertex_shader: src/vertex_shader.vert
	$(SLC) $< -o vertex_shader.spv

fragment_shader: src/fragment_shader.frag
	$(SLC) $< -o fragment_shader.spv

area_mesh: src/area_mesh.comp
	$(SLC) $< -o area_mesh.spv
