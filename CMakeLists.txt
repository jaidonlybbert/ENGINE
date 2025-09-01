include(CMakePrintHelpers)
include(FetchContent)

# Create a "compile_commands.json" file to inform LSP about build configuration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Modify these!
set(ENG_PUBLISHER "ENGINE_PUBLISHER")
set(ENG_PROJECT_NAME "ENGINE")
set(ENG_PROJECT_VERSION "0.0.1")

cmake_minimum_required(VERSION 3.22)
if (APPLE)
	set(VCPKG_TARGET_TRIPLET "arm64-osx")
elseif (WIN32)
	set(VCPKG_TARGET_TRIPLET "x64-windows")
endif()

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

project(Engine VERSION 0.0.1)
set(PROJECT_NAME_AND_VERSION "${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT "$ENV{VULKAN_SDK}" STREQUAL "")
	set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE INTERNAL "Copied from environment variable")
endif()

if (NOT "$ENV{DYLD_LIBRARY_PATH}" STREQUAL "")
	set(DYLD_LIBRARY_PATH "$ENV{DYLD_LIBRARY_PATH}" CACHE INTERNAL "Copied from environment variable")
	cmake_print_variables(DYLD_LIBRARY_PATH)
endif()

# Compile shaders to SPIR-V with GLSLC
# Find glslc from Vulkan SDK
find_program(GLSLC_EXECUTABLE
    NAMES glslc
    HINTS
        ENV VULKAN_SDK
    PATH_SUFFIXES bin
    DOC "Path to the glslc shader compiler from the Vulkan SDK"
    REQUIRED
)

# Print result
if(GLSLC_EXECUTABLE)
    message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")
else()
    message(FATAL_ERROR "glslc not found. Make sure Vulkan SDK is installed and VULKAN_SDK is set.")
endif()

set(shader_path ${CMAKE_HOME_DIRECTORY}/shaders/)
file(GLOB shaders RELATIVE ${CMAKE_SOURCE_DIR} "${shader_path}*.vert"
	"${shader_path}*.frag")
foreach(shader ${shaders})
	set(input_glsl "${CMAKE_HOME_DIRECTORY}/${shader}")
	set(output_spv "${input_glsl}.spv")
	add_custom_command(
		OUTPUT "${output_spv}"
		COMMAND "${GLSLC_EXECUTABLE}" "${input_glsl}" "-o" "${output_spv}"
		DEPENDS "${input_glsl}"
	VERBATIM)
	list(APPEND shader_outputs ${CMAKE_HOME_DIRECTORY}/${shader}.spv)
endforeach()
add_custom_target(shader_compile ALL DEPENDS ${shader_outputs} VERBATIM)

# Compile and link cpp source
configure_file(${PROJECT_SOURCE_DIR}/src/EngineConfig.h.in EngineConfig.h)

file(GLOB IMGUI_SOURCES "${PROJECT_SOURCE_DIR}/third_party/imgui/*.cpp")
add_executable(Engine ${PROJECT_SOURCE_DIR}/src/main.cpp ${IMGUI_SOURCES} ${PROJECT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_glfw.cpp ${PROJECT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_vulkan.cpp)

find_package(glfw3 CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

if (WIN32)
	# Tracy profiler doesn't build on ARM64 MacOS, the client does build however,
	# so maybe another machine could be used for the profiler over the network
	option(TRACY_ENABLE "" ON)
	option(TRACY_ON_DEMAND "" ON)
	add_subdirectory("${PROJECT_SOURCE_DIR}/third_party/tracy")
endif()

if (WIN32)
	find_path(WINREG_INCLUDE_DIRS "winreg/WinReg.hpp")
	target_include_directories(Engine PUBLIC ${WINREG_INCLUDE_DIRS})
endif()

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory("${PROJECT_SOURCE_DIR}/third_party/tinygltf")

target_include_directories(Engine PUBLIC 
			   "${PROJECT_BINARY_DIR}"
			   "${VULKAN_SDK}/Include")
target_include_directories(Engine PRIVATE ${Stb_INCLUDE_DIR})
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/third_party/imgui")
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/third_party/imgui/backends")
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/third_party/tinygltf")

cmake_print_variables(DYLD_LIBRARY_PATH)

target_link_libraries(Engine PRIVATE tinyobjloader::tinyobjloader)
target_link_libraries(Engine PUBLIC glfw Boost::filesystem glm::glm)
target_link_libraries(Engine PRIVATE nlohmann_json::nlohmann_json)
if (WIN32)
	target_link_libraries(Engine PUBLIC "${VULKAN_SDK}/Lib/vulkan-1.lib")
	target_link_libraries(Engine PUBLIC Tracy::TracyClient)
elseif (APPLE)
	target_link_libraries(Engine PUBLIC "${DYLD_LIBRARY_PATH}/libvulkan.1.dylib")
endif()

cmake_print_variables(PROJECT_BINARY_DIR)
cmake_print_variables(PROJECT_SOURCE_DIR)
cmake_print_variables(VULKAN_SDK)
cmake_print_variables(shader_outputs)
cmake_print_variables(output_spv)

# Set Install Target
if (WIN32)
	install(TARGETS Engine
	    COMPONENT Engine
	    RUNTIME_DEPENDENCIES
	    PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
	    POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
	    DIRECTORIES $<TARGET_FILE_DIR:Engine>)
elseif (APPLE)
	install(TARGETS Engine
	    RUNTIME ARCHIVE LIBRARY RUNTIME 
	    FRAMEWORK DESTINATION ./frameworks
	    BUNDLE PUBLIC_HEADER RESOURCE)
endif()
