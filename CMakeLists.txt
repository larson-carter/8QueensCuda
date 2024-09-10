cmake_minimum_required(VERSION 3.29)
project(8QueensCuda)

set(CMAKE_CXX_STANDARD 26)

# Add optimization flags for release builds
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -pthread")

add_executable(8QueensCuda main.cpp
)
