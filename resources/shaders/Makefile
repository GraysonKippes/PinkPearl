SLC = glslc

shaders: compute_matrices room_texture VertexShader FragmentShader VertexShaderLines FragmentShaderLines

compute_matrices: src/compute_matrices.comp
	$(SLC) $< -o compute_matrices.spv

room_texture: src/room_texture.comp
	$(SLC) $< -o room_texture.spv

VertexShader: src/VertexShader.vert
	$(SLC) $< -o VertexShader.spv

FragmentShader: src/FragmentShader.frag
	$(SLC) $< -o FragmentShader.spv

VertexShaderLines: src/VertexShaderLines.vert
	$(SLC) $< -o VertexShaderLines.spv

FragmentShaderLines: src/FragmentShaderLines.frag
	$(SLC) $< -o FragmentShaderLines.spv
