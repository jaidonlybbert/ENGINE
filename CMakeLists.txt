include(CMakePrintHelpers)
include(FetchContent)

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

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/shaders")

# Build config header
set(Engine_INSTALL_DIR ${CMAKE_HOME_DIRECTORY})  # used to resolve paths to assets at runtime
configure_file(${PROJECT_SOURCE_DIR}/src/EngineConfig.hpp.in EngineConfig.hpp)

# file(GLOB_RECURSE SOURCE_FILES CMAKE_CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/src/*.cpp")
set(SOURCE_FILES
	"${PROJECT_SOURCE_DIR}/src/HeaderLibs.cpp"
	"${PROJECT_SOURCE_DIR}/src/main.cpp"
	"${PROJECT_SOURCE_DIR}/src/application/Application.cpp"
	"${PROJECT_SOURCE_DIR}/src/filesystem/FilesystemInterface.cpp"
	"${PROJECT_SOURCE_DIR}/src/gui/Gui.cpp"
	"${PROJECT_SOURCE_DIR}/src/hid/Input.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Buffer.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Command.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Device.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Image.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Instance.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/PhysicalDevice.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Renderer.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Swapchain.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Utils.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/pipelines/Pipeline.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/pipelines/PipelineFactory.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/pipelines/PipelinePosColTex.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/pipelines/PipelineUtils.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/pipelines/ShaderFactory.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/DFT.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/DrawData.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/Gltf.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/Mesh.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/Mesh2.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/Obj.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/Scene.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/VulkanAdapter.cpp"
	"${PROJECT_SOURCE_DIR}/src/scenes/ProceduralGeometry.cpp"
	"${PROJECT_SOURCE_DIR}/src/scenes/SceneWorld.cpp"
	"${PROJECT_SOURCE_DIR}/src/sockets/CMakeLists.txt"
	#	"${PROJECT_SOURCE_DIR}/sockets/SocketSessionServer.cpp"
)

file(GLOB_RECURSE HEADER_FILES CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.hpp")
add_executable(Engine ${SOURCE_FILES})
target_sources(${PROJECT_NAME} PRIVATE ${HEADER_FILES} ${PROJECT_SOURCE_DIR}/src/EngineConfig.hpp.in)

# File groups for MSVC IDE integration
source_group(TREE ${PROJECT_SOURCE_DIR}/src PREFIX "Source" FILES ${SOURCE_FILES} ${PROJECT_SOURCE_DIR}/src/EngineConfig.hpp.in)
source_group(TREE ${PROJECT_SOURCE_DIR}/include PREFIX "Header Files" FILES ${HEADER_FILES})

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

add_subdirectory("${PROJECT_SOURCE_DIR}/third_party")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/sockets")

target_include_directories(Engine PUBLIC "${PROJECT_BINARY_DIR}")
target_include_directories(Engine PRIVATE ${Stb_INCLUDE_DIR})
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include") 
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include/pipelines")  
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include/primitives")
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include/interfaces")

target_link_libraries(Engine PRIVATE tinyobjloader::tinyobjloader)
target_link_libraries(Engine PUBLIC glfw glm::glm)
target_link_libraries(Engine PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(Engine PRIVATE pmp::pmp)
target_link_libraries(Engine PRIVATE lua::lua)
target_link_libraries(Engine PRIVATE Vulkan::Vulkan)
target_link_libraries(Engine PRIVATE flatbuffers::flatbuffers)
target_link_libraries(Engine PRIVATE asio::asio)
target_link_libraries(Engine PRIVATE tinygltf)
target_link_libraries(Engine PRIVATE imgui)
target_link_libraries(Engine PUBLIC sockets)

if (WIN32)
	target_link_libraries(Engine PUBLIC Tracy::TracyClient)
	# Target Windows 10 or later
	target_compile_definitions(Engine PRIVATE _WIN32_WINNT=0x0A00)
endif()

cmake_print_variables(DYLD_LIBRARY_PATH)
cmake_print_variables(PROJECT_BINARY_DIR)
cmake_print_variables(PROJECT_SOURCE_DIR)
cmake_print_variables(VULKAN_SDK)
