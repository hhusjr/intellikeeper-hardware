# cmake_minimum_required(VERSION <specify CMake version here>)
project(intellikeeper_base)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lwiringPi -fsanitize=address  -static-libasan")

include_directories(network)
include_directories(node)

add_library(network_raspi network/protocol.h network/protocol.cpp)
add_library(cc1100_raspi network/cc1100_raspi.h network/cc1100_raspi.cpp)
target_link_libraries(network_raspi cc1100_raspi)

add_library(manager node/manager.h node/manager.cpp)
target_link_libraries(manager network_raspi)

add_executable(intellikeeper_base intellikeeper.cpp)
target_link_libraries(intellikeeper_base network_raspi manager)

