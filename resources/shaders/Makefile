SLC = glslc
SLCFLAGS = --target-env=vulkan1.3 --target-spv=spv1.6
SRC_DIR = src

shaders: compute_matrices.spv ComputeMatrices.spv room_texture.spv RoomTexture.spv \
			VertexShader.spv FragmentShader.spv \
			VertexShaderLines.spv FragmentShaderLines.spv 

%.spv: $(SRC_DIR)/%.vert
	$(SLC) $(SLCFLAGS) $< -o $@

%.spv: $(SRC_DIR)/%.frag
	$(SLC) $(SLCFLAGS) $< -o $@

%.spv: $(SRC_DIR)/%.comp
	$(SLC) $(SLCFLAGS) $< -o $@
