include(CMakePrintHelpers)
include(FetchContent)

cmake_minimum_required(VERSION 3.22)

set(VCPKG_TARGET_TRIPLET "x64-windows")
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

project(Engine VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT "$ENV{VULKAN_SDK}" STREQUAL "")
    set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE INTERNAL "Copied from environment variable")
endif()

# Compile shaders to SPIR-V with GLSLC
find_program(GLSLC glslc)
set(shader_path ${CMAKE_HOME_DIRECTORY}/shaders/)
file(GLOB shaders RELATIVE ${CMAKE_SOURCE_DIR} "${shader_path}*.vert"
	"${shader_path}*.frag")
foreach(shader ${shaders})
	set(input_glsl "${CMAKE_HOME_DIRECTORY}/${shader}")
	set(output_spv "${input_glsl}.spv")
	add_custom_command(
		OUTPUT "${output_spv}"
		COMMAND "glslc" "${input_glsl}" "-o" "${output_spv}"
		DEPENDS "${input_glsl}"
		VERBATIM)
	list(APPEND shader_outputs ${CMAKE_HOME_DIRECTORY}/${shader}.spv)
endforeach()
add_custom_target(shader_compile ALL DEPENDS ${shader_outputs} VERBATIM)

# Compile and link cpp source
configure_file(${PROJECT_SOURCE_DIR}/src/EngineConfig.h.in EngineConfig.h)

add_executable(Engine ${PROJECT_SOURCE_DIR}/src/main.cpp)

find_package(glfw3 CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)

target_include_directories(Engine PUBLIC 
			   "${PROJECT_BINARY_DIR}"
			   "${VULKAN_SDK}/Include"
			   )

target_link_libraries(Engine PUBLIC 
			glfw
			"${VULKAN_SDK}/Lib/vulkan-1.lib"
			Boost::filesystem)

cmake_print_variables(PROJECT_BINARY_DIR)
cmake_print_variables(PROJECT_SOURCE_DIR)
cmake_print_variables(VULKAN_SDK)
cmake_print_variables(shader_outputs)
cmake_print_variables(output_spv)
