cmake_minimum_required(VERSION 3.10)
project(relay)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
include_directories(include)
link_directories(./lib)
add_executable(relay main.cpp Tunnel.h)
target_link_libraries(relay libmuduo_base.a libmuduo_net.a)