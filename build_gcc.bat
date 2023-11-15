@ECHO OFF
MD build_gcc
CD build_gcc
cmake ..\ -G "MSYS Makefiles" -D CMAKE_C_COMPILER=gcc
cmake --build .
PAUSE
