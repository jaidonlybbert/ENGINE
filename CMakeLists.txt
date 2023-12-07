cmake_minimum_required(VERSION 3.28)
project(Engine VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

configure_file(./src/EngineConfig.h.in EngineConfig.h)

add_executable(Engine ./src/main.cpp)

target_include_directories(Engine PUBLIC "${PROJECT_BINARY_DIR}")