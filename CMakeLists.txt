include(CMakePrintHelpers)
include(FetchContent)

# Modify these!
set(ENG_PUBLISHER "ENGINE_PUBLISHER")
set(ENG_PROJECT_NAME "ENGINE")
set(ENG_PROJECT_VERSION "0.0.1")

cmake_minimum_required(VERSION 3.25.1)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/build/generators/conan_toolchain.cmake"
	CACHE STRING "Conan toolchain file")

project(Engine VERSION 0.0.1)
set(PROJECT_NAME_AND_VERSION "${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_VERSION}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_HOME_DIRECTORY}/build/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_HOME_DIRECTORY}/build/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_HOME_DIRECTORY}/build/MinSizeRel)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_HOME_DIRECTORY}/build/RelWithDebInfo)

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

set(Engine_INSTALL_DIR ${CMAKE_HOME_DIRECTORY})
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
target_sources(shader_compile PRIVATE ${shaders})

# Compile and link cpp source
configure_file(${PROJECT_SOURCE_DIR}/src/EngineConfig.hpp.in EngineConfig.hpp)

file(GLOB IMGUI_SOURCES "${PROJECT_SOURCE_DIR}/third_party/imgui/*.cpp")
file(GLOB_RECURSE SOURCE_FILES CMAKE_CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER_FILES CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.hpp")
add_executable(Engine ${SOURCE_FILES} ${IMGUI_SOURCES} ${PROJECT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_glfw.cpp ${PROJECT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_vulkan.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${HEADER_FILES} ${PROJECT_SOURCE_DIR}/src/EngineConfig.hpp.in)


# File groups for MSVC IDE integration
source_group(TREE ${PROJECT_SOURCE_DIR}/src PREFIX "Source" FILES ${SOURCE_FILES} ${PROJECT_SOURCE_DIR}/src/EngineConfig.hpp.in)
source_group(TREE ${PROJECT_SOURCE_DIR}/include PREFIX "Header Files" FILES ${HEADER_FILES})
source_group("Shaders" FILES ${shaders})
source_group("ImGui" FILES ${IMGUI_SOURCES})

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(nlohmann_json 3.12 CONFIG REQUIRED)
find_package(pmp REQUIRED)
find_package(lua REQUIRED)
find_package(Vulkan REQUIRED)
find_package(flatbuffers REQUIRED)
find_package(asio REQUIRED)

#if (WIN32)
#	find_path(WINREG_INCLUDE_DIRS "winreg/WinReg.hpp")
#	target_include_directories(Engine PUBLIC ${WINREG_INCLUDE_DIRS})
#endif()

add_subdirectory("${PROJECT_SOURCE_DIR}/third_party")

target_include_directories(Engine PUBLIC "${PROJECT_BINARY_DIR}")
target_include_directories(Engine PRIVATE ${Stb_INCLUDE_DIR})
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/third_party/imgui")
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/third_party/imgui/backends")
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include") 
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include/pipelines")  
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include/primitives")
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include/interfaces")

cmake_print_variables(DYLD_LIBRARY_PATH)

target_link_libraries(Engine PRIVATE tinyobjloader::tinyobjloader)
target_link_libraries(Engine PUBLIC glfw glm::glm)
target_link_libraries(Engine PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(Engine PRIVATE pmp::pmp)
target_link_libraries(Engine PRIVATE lua::lua)
target_link_libraries(Engine PRIVATE Vulkan::Vulkan)
target_link_libraries(Engine PRIVATE flatbuffers::flatbuffers)
target_link_libraries(Engine PRIVATE asio::asio)
target_link_libraries(Engine PRIVATE tinygltf)

if (WIN32)
	target_link_libraries(Engine PUBLIC Tracy::TracyClient)
	# Target Windows 10 or later
	target_compile_definitions(Engine PRIVATE _WIN32_WINNT=0x0A00)
endif()

cmake_print_variables(PROJECT_BINARY_DIR)
cmake_print_variables(PROJECT_SOURCE_DIR)
cmake_print_variables(VULKAN_SDK)
cmake_print_variables(shader_outputs)
cmake_print_variables(output_spv)
cmake_print_variables(Engine_INSTALL_DIR)

# Set Install Target
# if (WIN32)
# 	install(TARGETS Engine
# 	    COMPONENT Engine
# 	    RUNTIME_DEPENDENCIES
# 	    PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
# 	    POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
# 	    DIRECTORIES $<TARGET_FILE_DIR:Engine>)
# elseif (APPLE)
# 	install(TARGETS Engine
# 	    RUNTIME ARCHIVE LIBRARY RUNTIME 
# 	    FRAMEWORK DESTINATION ./frameworks
# 	    BUNDLE PUBLIC_HEADER RESOURCE)
# endif()
