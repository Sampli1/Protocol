cmake_minimum_required(VERSION 3.0)
project(DemoProtocol)

find_library(wiringPi_LIB wiringPi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_SOURCE_DIR}/include)


add_library(SER src/serial.cpp)
add_library(NP src/nucleo_protocol.cpp)

add_executable(myapp src/main.cpp)

target_link_libraries(SER ${wiringPi_LIB})
target_link_libraries(NP SER)
target_link_libraries(myapp SER ${wiringPi_LIB} NP)