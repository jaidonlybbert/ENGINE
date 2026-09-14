include(CMakePrintHelpers)
include(FetchContent)

set(ENG_PUBLISHER "ENGINE_PUBLISHER")
set(ENG_PROJECT_NAME "ENGINE")
set(ENG_PROJECT_VERSION "0.0.1")

cmake_minimum_required(VERSION 3.25.1)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/build/generators/conan_toolchain.cmake"
	CACHE STRING "Conan toolchain file")

project(Engine VERSION 0.0.1)

# Enable CTest at the top level so `ctest --test-dir build` picks up the tests
# registered under test/ (gtest_discover_tests).
enable_testing()

set(PROJECT_NAME_AND_VERSION "${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_VERSION}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_HOME_DIRECTORY}/build/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_HOME_DIRECTORY}/build/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_HOME_DIRECTORY}/build/MinSizeRel)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_HOME_DIRECTORY}/build/RelWithDebInfo)

if (NOT "$ENV{VULKAN_SDK}" STREQUAL "")
	set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE INTERNAL "Copied from environment variable")
	cmake_print_variables(VULKAN_SDK)
endif()

if (NOT "$ENV{DYLD_LIBRARY_PATH}" STREQUAL "")
	set(DYLD_LIBRARY_PATH "$ENV{DYLD_LIBRARY_PATH}" CACHE INTERNAL "Copied from environment variable")
	cmake_print_variables(DYLD_LIBRARY_PATH)
endif()

if (NOT "$ENV{VK_DRIVER_FILES}" STREQUAL "")
	set(VK_DRIVER_FILES "$ENV{VK_DRIVER_FILES}" CACHE INTERNAL "Copied from environment variable")
	cmake_print_variables(VK_DRIVER_FILES)
endif()

if (NOT "$ENV{Vulkan_LIBRARY}" STREQUAL "")
	set(Vulkan_LIBRARY "$ENV{Vulkan_LIBRARY}" CACHE INTERNAL "Copied from environment variable")
	cmake_print_variables(Vulkan_LIBRARY)
endif()

if (NOT "$ENV{Vulkan_INCLUDE_DIR}" STREQUAL "")
	set(Vulkan_INCLUDE_DIR "$ENV{Vulkan_INCLUDE_DIR}" CACHE INTERNAL "Copied from environment variable")
	cmake_print_variables(Vulkan_INCLUDE_DIR)
endif()

# The Android build (see issue #10/#46) is kept deliberately separate and minimal for
# now, rather than threading ANDROID checks through the desktop Engine target's ~100
# lines of add_subdirectory/find_package/target_link_libraries below: almost everything
# there is GLFW-specific (window, hid/glfw, vk::renderer's imgui bridge) or untested on
# Android (Jolt, PMP, VMA via Conan), and unpicking that piece by piece here would just
# duplicate the follow-up issues (#47-#51) that already exist to replace each GLFW-coupled
# piece with an Android equivalent. This block only proves the Conan/NDK/CMake toolchain
# and dependency graph work end to end (android_native_app_glue + a log line) - widening
# it to link more of the engine's already-portable libraries (engine::scene,
# engine::physics, engine::filesystem, etc.) is expected, natural follow-up as those
# issues land, not something to force here.
if (ANDROID)
	add_library(native_app_glue STATIC "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")
	target_include_directories(native_app_glue PUBLIC "${ANDROID_NDK}/sources/android/native_app_glue")

	add_library(Engine SHARED "${PROJECT_SOURCE_DIR}/src/main_android.cpp")
	target_link_libraries(Engine PRIVATE native_app_glue android log)
	# Required so the OS can find the entry point native_app_glue implements.
	target_link_options(Engine PRIVATE "-u" "ANativeActivity_onCreate")
else()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/shaders")

set(SOURCE_FILES
	"${PROJECT_SOURCE_DIR}/src/HeaderLibs.cpp"
	"${PROJECT_SOURCE_DIR}/src/main.cpp"
)

file(GLOB_RECURSE HEADER_FILES CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.hpp")
add_executable(Engine ${SOURCE_FILES})
target_sources(${PROJECT_NAME} PRIVATE ${HEADER_FILES})

# File groups for MSVC IDE integration
source_group(TREE ${PROJECT_SOURCE_DIR}/src PREFIX "Source" FILES ${SOURCE_FILES})
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
find_package(VulkanMemoryAllocator REQUIRED)
find_package(Jolt REQUIRED)
find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)

add_subdirectory("${PROJECT_SOURCE_DIR}/include")
add_subdirectory("${PROJECT_SOURCE_DIR}/third_party")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/sockets")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/logger")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/application")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/config")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/filesystem")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/gui")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/guis")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/scene")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/scenes/common")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/scenes/world")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/scenes/blue_sky")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/renderer")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/hid")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/hid/glfw")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/window/glfw")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/events")
add_subdirectory("${PROJECT_SOURCE_DIR}/src/physics")
add_subdirectory("${PROJECT_SOURCE_DIR}/test")

target_include_directories(Engine PUBLIC "${PROJECT_BINARY_DIR}")
target_include_directories(Engine PRIVATE ${Stb_INCLUDE_DIR})
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include") 
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include/pipelines")  
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include/primitives")
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/include/interfaces")

target_link_libraries(Engine PRIVATE tinyobjloader::tinyobjloader)
# No longer linked directly - engine::vk::renderer (imgui_impl_glfw) and
# engine::window::glfw/engine::hid::glfw already bring GLFW in transitively (see issue
# #42), and nothing in Engine's own sources (main.cpp) calls into GLFW directly anymore.
target_link_libraries(Engine PUBLIC glm::glm)
target_link_libraries(Engine PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(Engine PRIVATE pmp::pmp)
target_link_libraries(Engine PRIVATE lua::lua)
target_link_libraries(Engine PRIVATE Vulkan::Vulkan)
target_link_libraries(Engine PRIVATE flatbuffers::flatbuffers)
target_link_libraries(Engine PRIVATE asio::asio)
target_link_libraries(Engine PRIVATE tinygltf)
target_link_libraries(Engine PRIVATE imgui)
# target_link_libraries(Engine PRIVATE dynalo)
target_link_libraries(Engine PRIVATE GPUOpen::VulkanMemoryAllocator)
target_link_libraries(Engine PRIVATE Jolt::Jolt)

target_link_libraries(Engine PUBLIC engine::sockets)
target_link_libraries(Engine PUBLIC engine::logger)
target_link_libraries(Engine PUBLIC engine::application)
target_link_libraries(Engine PUBLIC engine::config)
target_link_libraries(Engine PUBLIC engine::filesystem_interface)
target_link_libraries(Engine PUBLIC engine::filesystem)
target_link_libraries(Engine PUBLIC engine::gui)
target_link_libraries(Engine PUBLIC engine::renderer_interface)
target_link_libraries(Engine PUBLIC engine::vk::renderer)
target_link_libraries(Engine PUBLIC engine::vk::adapter)
target_link_libraries(Engine PUBLIC engine::scene)
target_link_libraries(Engine PUBLIC engine::window_interface)
target_link_libraries(Engine PUBLIC engine::window::glfw)
target_link_libraries(Engine PUBLIC engine::hid_interface)
target_link_libraries(Engine PUBLIC engine::hid)
target_link_libraries(Engine PUBLIC engine::hid::glfw)
target_link_libraries(Engine PUBLIC engine::events)
target_link_libraries(Engine PUBLIC engine::physics)

target_link_libraries(Engine PUBLIC engine::scene::gui)
# The application links every scene it wants to offer at startup (see main.cpp's
# initializeScene()). Neither scene library depends on the other.
target_link_libraries(Engine PUBLIC engine::scenes::world)
target_link_libraries(Engine PUBLIC engine::scenes::blue_sky)

if (WIN32)
	target_link_libraries(Engine PUBLIC Tracy::TracyClient)
	# Target Windows 10 or later
	target_compile_definitions(Engine PRIVATE _WIN32_WINNT=0x0A00)
endif()

cmake_print_variables(DYLD_LIBRARY_PATH)
cmake_print_variables(PROJECT_BINARY_DIR)
cmake_print_variables(PROJECT_SOURCE_DIR)
cmake_print_variables(VULKAN_SDK)

endif()
