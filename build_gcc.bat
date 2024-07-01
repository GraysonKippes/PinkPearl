@ECHO OFF
MD build_gcc
CD build_gcc
cmake ..\ -G "MSYS Makefiles" -DCMAKE_C_COMPILER=gcc
cmake --build .
PAUSE
