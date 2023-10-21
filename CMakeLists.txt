cmake_minimum_required(VERSION 3.27)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

project(PinkPearl)
add_executable(PinkPearl)

target_compile_options(PinkPearl PRIVATE -Wall -Wextra -Wpedantic -g)

find_package(Vulkan REQUIRED)

target_link_libraries(PinkPearl PRIVATE Vulkan::Vulkan)

target_link_libraries(PinkPearl PRIVATE 
	"${PROJECT_SOURCE_DIR}/lib/libglfw3.a"
	"${PROJECT_SOURCE_DIR}/lib/portaudio.lib"
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
	src/render/vulkan/buffer.c
	src/render/vulkan/command_buffer.c
	src/render/vulkan/compute_pipeline.c
	src/render/vulkan/descriptor.c
	src/render/vulkan/graphics_pipeline.c
	src/render/vulkan/logical_device.c
	src/render/vulkan/physical_device.c
	src/render/vulkan/shader.c
	src/render/vulkan/swapchain.c
	src/render/vulkan/vertex_input.c
	src/render/vulkan/vulkan_instance.c
	src/render/vulkan/vulkan_manager.c
)
