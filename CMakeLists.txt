cmake_minimum_required(VERSION 3.0)
project(DemoProtocol)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(UTILS src/protocol_utils.cpp)
add_library(SER src/serial.cpp)
add_library(NP src/nucleo_protocol.cpp)


add_executable(myapp test/test.cpp)

target_link_libraries(NP SER UTILS)
target_link_libraries(myapp SER NP UTILS)