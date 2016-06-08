cmake_minimum_required(VERSION 3.3)
project(ex5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES Server.cpp Server.h Client.cpp Client.h)
add_executable(ex5 ${SOURCE_FILES})