cmake_minimum_required(VERSION 3.27)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

project(PinkPearl)
add_executable(PinkPearl)
target_sources(PinkPearl PRIVATE src/main.c)
