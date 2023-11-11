cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

project(PinkPearl)
add_executable(PinkPearl)

#target_compile_options(PinkPearl PRIVATE /TC /std:c17 /options:strict /W3 /we4013 /we4133 /fsanitize=address)
target_compile_options(PinkPearl PRIVATE -std=c17 -Wpedantic -g)

find_package(Vulkan REQUIRED)

target_link_libraries(PinkPearl PRIVATE 
	Vulkan::Vulkan 
	#"${PROJECT_SOURCE_DIR}/lib/glfw3.lib"
	"${PROJECT_SOURCE_DIR}/lib/libglfw3.a"
	#"${PROJECT_SOURCE_DIR}/lib/portaudio.lib"
)

target_include_directories(PinkPearl PRIVATE
	"${PROJECT_SOURCE_DIR}/src"
	"${PROJECT_SOURCE_DIR}/include"
)


target_sources(PinkPearl PRIVATE 
	src/debug.c
	src/main.c
	src/glfw/glfw_manager.c
	src/log/logging.c
	src/render/model.c
	src/render/render_object.c
	src/render/renderer.c
	src/render/math/vector3F.c
	src/render/stb/image_data.c
	src/render/vulkan/buffer.c
	src/render/vulkan/command_buffer.c
	src/render/vulkan/compute_pipeline.c
	src/render/vulkan/descriptor.c
	src/render/vulkan/frame.c
	src/render/vulkan/graphics_pipeline.c
	src/render/vulkan/image.c
	src/render/vulkan/logical_device.c
	src/render/vulkan/physical_device.c
	src/render/vulkan/queue.c
	src/render/vulkan/shader.c
	src/render/vulkan/swapchain.c
	src/render/vulkan/vertex_input.c
	src/render/vulkan/vulkan_instance.c
	src/render/vulkan/vulkan_manager.c
	src/util/string_array.c
)
