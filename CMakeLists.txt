cmake_minimum_required(VERSION 3.27)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

project(PinkPearl)
add_executable(PinkPearl)

target_link_libraries(PinkPearl PRIVATE 
	"${PROJECT_SOURCE_DIR}/lib/libglfw3.a"
	"${PROJECT_SOURCE_DIR}/lib/vulkan-1.lib"
	"${PROJECT_SOURCE_DIR}/lib/portaudio.lib")

target_include_directories(PinkPearl PRIVATE
	"${PROJECT_SOURCE_DIR}/src"
	"${PROJECT_SOURCE_DIR}/include")

target_sources(PinkPearl PRIVATE 
	src/main.c
	src/glfw/glfw_manager.c
	src/render/vulkan/physical_device.c)

