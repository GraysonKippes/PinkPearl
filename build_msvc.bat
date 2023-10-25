@ECHO OFF
MD build_msvc
CD build_msvc
cmake ../ -G "Visual Studio 17 2022"
cmake --build .
PAUSE
