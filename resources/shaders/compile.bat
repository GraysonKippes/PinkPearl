@ECHO OFF
ECHO Compiling vertex shader...
glslc.exe test_shader_vertex.vert -o vert.spv
ECHO Compiling fragment shader...
glslc.exe test_shader_fragment.frag -o frag.spv
ECHO Compiled shaders!
pause