@ECHO OFF
MD build
CD build
cmake ../
cmake --build .
CD debug
