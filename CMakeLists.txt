include(CMakePrintHelpers)
include(FetchContent)

cmake_minimum_required(VERSION 3.28)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

project(Engine VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

configure_file(${PROJECT_SOURCE_DIR}/src/EngineConfig.h.in EngineConfig.h)

add_executable(Engine ${PROJECT_SOURCE_DIR}/src/main.cpp)

target_include_directories(Engine PUBLIC 
			   "${PROJECT_BINARY_DIR}"
			   "${VULKAN_SDK}/Include")

cmake_print_variables(PROJECT_BINARY_DIR)
cmake_print_variables(PROJECT_SOURCE_DIR)