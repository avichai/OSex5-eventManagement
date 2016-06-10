cmake_minimum_required(VERSION 3.3)
project(ex5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES Server.cpp Utils.cpp)
add_executable(ex5 ${SOURCE_FILES})