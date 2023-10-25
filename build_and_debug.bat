@ECHO OFF
MD build
CD build
cmake ../ -G "MSYS Makefiles" -D CMAKE_C_COMPILER=gcc
cmake --build .
gdb PinkPearl.exe
PAUSE
