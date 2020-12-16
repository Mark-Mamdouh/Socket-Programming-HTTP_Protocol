cmake_minimum_required(VERSION 3.12)
project(HTTP_Protocol)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS -pthread)

add_executable(HTTP_Protocol main.cpp cmake-build-debug)