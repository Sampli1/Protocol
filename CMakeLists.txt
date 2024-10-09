cmake_minimum_required(VERSION 3.0)
project(DemoProtocol)

find_library(wiringPi_LIB wiringPi)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_library(SER src/serial.cpp)

add_executable(myapp src/main.cpp)

target_link_libraries(SER ${wiringPi_LIB})
target_link_libraries(myapp SER ${wiringPi_LIB})