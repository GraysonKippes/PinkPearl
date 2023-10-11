@ECHO OFF
MD build
CD build
cmake ../ -G "MSYS Makefiles"
cmake --build .
CD debug
PinkPearl.exe
PAUSE
