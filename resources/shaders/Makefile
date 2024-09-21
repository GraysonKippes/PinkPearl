SLC = glslc
SRC_DIR = src

shaders: compute_matrices.spv room_texture.spv \
			VertexShader.spv FragmentShader.spv \
			VertexShaderLines.spv FragmentShaderLines.spv \
			VertexShaderBindless.spv FragmentShaderBindless.spv

%.spv: $(SRC_DIR)/%.vert
	$(SLC) $< -o $@.spv

%.spv: $(SRC_DIR)/%.frag
	$(SLC) $< -o $@.spv

%.spv: $(SRC_DIR)/%.comp
	$(SLC) $< -o $@.spv
