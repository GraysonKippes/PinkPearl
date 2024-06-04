cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

project(PinkPearl VERSION 0.0)
add_executable(PinkPearl)

set(APP_NAME "\"Pink Pearl\"")
set(RESOURCE_PATH "\"../resources/\"")
configure_file(src/config.h.in config.h)

set(COMPILE_OPTIONS_MSVC, "/TC /std:c17 /options:strict /W3 /we4013 /we4133 /fsanitize=address")

target_compile_options(PinkPearl PRIVATE -std=c2x -lpthread -g -Wpedantic -Wall -Wextra
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
	-Wno-switch
)

find_package(Vulkan REQUIRED)

target_link_libraries(PinkPearl PRIVATE Vulkan::Vulkan 
	"${PROJECT_SOURCE_DIR}/lib/libportaudio.dll" 
	"${PROJECT_SOURCE_DIR}/lib/libglfw3.a"
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
	src/game/game.c
	src/audio/audio_config.c
	src/audio/audio_loader.c
	src/audio/audio_mixer.c
	src/audio/audio_queue.c
	src/audio/portaudio/portaudio_manager.c
	src/game/area/area.c
	src/game/area/area_extent.c
	src/game/area/fgm_file_parse.c
	src/game/area/room.c
	src/game/entity/entity.c
	src/game/entity/entity_ai.c
	src/game/entity/entity_manager.c
	src/game/entity/entity_registry.c
	src/glfw/glfw_manager.c
	src/glfw/input_manager.c
	src/log/error_code.c
	src/log/log_stack.c
	src/log/logging.c
	src/math/extent.c
	src/math/hitbox.c
	src/math/offset.c
	src/math/vector3D.c
	src/render/area_render_state.c
	src/render/render_config.c
	src/render/render_object.c
	src/render/renderer.c
	src/render/texture_info.c
	src/render/texture_state.c
	src/render/stb/image_data.c
	src/render/vulkan/buffer.c
	src/render/vulkan/command_buffer.c
	src/render/vulkan/compute_pipeline.c
	src/render/vulkan/descriptor.c
	src/render/vulkan/frame.c
	src/render/vulkan/graphics_pipeline.c
	src/render/vulkan/image.c
	src/render/vulkan/logical_device.c
	src/render/vulkan/memory.c
	src/render/vulkan/physical_device.c
	src/render/vulkan/queue.c
	src/render/vulkan/shader.c
	src/render/vulkan/swapchain.c
	src/render/vulkan/synchronization.c
	src/render/vulkan/texture.c
	src/render/vulkan/texture_loader.c
	src/render/vulkan/texture_manager.c
	src/render/vulkan/vertex_input.c
	src/render/vulkan/vulkan_instance.c
	src/render/vulkan/vulkan_manager.c
	src/render/vulkan/vulkan_render.c
	src/render/vulkan/compute/compute_area_mesh.c
	src/render/vulkan/compute/compute_matrices.c
	src/render/vulkan/compute/compute_room_texture.c
	src/render/vulkan/math/lerp.c
	src/render/vulkan/math/render_vector.c
	src/render/vulkan/math/vector4F.c
	src/util/allocate.c
	src/util/bit.c
	src/util/file_io.c
	src/util/string.c
	src/util/string_array.c
	src/util/time.c
)
