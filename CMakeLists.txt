cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

project(PinkPearl VERSION 0.2)
add_executable(PinkPearl)

set(APP_NAME "\"Pink Pearl\"")
set(RESOURCE_PATH "\"../resources/\"")
configure_file(src/config.h.in config.h)

target_compile_options(PinkPearl PRIVATE -std=c2x -lpthread -g -fanalyzer -Wpedantic -Wall -Wextra
	-Werror=return-type 
	-Werror=implicit-function-declaration 
	-Werror=implicit-int
	-Werror=sign-compare
	-Werror=stringop-overread
	-Werror=incompatible-pointer-types
	-Werror=int-conversion
	-Werror=format 
	-Werror=bool-compare 
	-Werror=discarded-qualifiers 
	-Werror=address 
	-Werror=array-parameter
	-Werror=endif-labels
	-Werror=enum-conversion
	-Werror=free-nonheap-object
	-Werror=pointer-arith
	-Werror=cast-align=strict
	-Werror=strict-prototypes
	-Werror=stringop-overflow=4
	-Werror=shadow
	-Werror=strict-aliasing=3
	-Wno-switch
)

find_package(Vulkan REQUIRED)

target_link_libraries(PinkPearl PRIVATE 
	Vulkan::Vulkan 
	"${PROJECT_SOURCE_DIR}/lib/libglfw3.a"
	"${PROJECT_SOURCE_DIR}/lib/libportaudio.dll" 
	"${PROJECT_SOURCE_DIR}/lib/libDataStuff.a"
)

target_include_directories(PinkPearl PRIVATE
	"${PROJECT_BINARY_DIR}"
	"${PROJECT_SOURCE_DIR}/src"
	"${PROJECT_SOURCE_DIR}/include"
)

target_sources(PinkPearl PRIVATE 
	src/client.c
	src/debug.c
	src/main.c
	src/game/Game.c
	src/audio/audio_config.c
	src/audio/audio_loader.c
	src/audio/audio_mixer.c
	src/audio/audio_queue.c
	src/audio/portaudio/portaudio_manager.c
	src/game/area/area.c
	src/game/area/fgm_file_parse.c
	src/game/area/room.c
	src/game/entity/entity.c
	src/game/entity/EntityAI.c
	src/game/entity/entity_manager.c
	src/game/entity/EntityRegistry.c
	src/game/entity/EntitySpawner.c
	src/glfw/GLFWManager.c
	src/glfw/InputManager.c
	src/log/Logger.c
	src/math/Box.c
	src/math/extent.c
	src/math/offset.c
	src/math/Vector.c
	src/render/render_config.c
	src/render/RenderManager.c
	src/render/texture_pack.c
	src/render/stb/ImageData.c
	src/render/vulkan/buffer.c
	src/render/vulkan/Buffer2.c
	src/render/vulkan/CommandBuffer.c
	src/render/vulkan/ComputePipeline.c
	src/render/vulkan/Descriptor.c
	src/render/vulkan/Draw.c
	src/render/vulkan/frame.c
	src/render/vulkan/GraphicsPipeline.c
	src/render/vulkan/logical_device.c
	src/render/vulkan/memory.c
	src/render/vulkan/physical_device.c
	src/render/vulkan/Pipeline.c
	src/render/vulkan/queue.c
	src/render/vulkan/Shader.c
	src/render/vulkan/Swapchain.c
	src/render/vulkan/synchronization.c
	src/render/vulkan/texture.c
	src/render/vulkan/texture_loader.c
	src/render/vulkan/texture_manager.c
	src/render/vulkan/TextureState.c
	src/render/vulkan/vertex_input.c
	src/render/vulkan/vulkan_instance.c
	src/render/vulkan/VulkanManager.c
	src/render/vulkan/compute/ComputeMatrices.c
	src/render/vulkan/compute/ComputeStitchTexture.c
	src/render/vulkan/math/lerp.c
	src/render/vulkan/math/render_vector.c
	src/util/allocate.c
	src/util/file_io.c
	src/util/Random.c
	src/util/string.c
	src/util/string_array.c
	src/util/time.c
)
