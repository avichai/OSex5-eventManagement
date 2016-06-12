cmake_minimum_required(VERSION 3.3)
project(ex5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES Server.cpp Server.h  Client.h Server.cpp Utils.cpp main.cpp)
add_executable(ex5 ${SOURCE_FILES})