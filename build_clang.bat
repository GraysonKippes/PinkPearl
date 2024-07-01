@ECHO OFF
MD build_clang
CD build_clang
cmake ..\ -G "Visual Studio 17 2022" -T "clang_cl_x64_x64"
cmake --build .
PAUSE
