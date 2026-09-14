# ---- TRACY ---- #
if (WIN32)
	# Tracy profiler doesn't build on ARM64 MacOS, the client does build however,
	# so maybe another machine could be used for the profiler over the network
	option(TRACY_ENABLE "" ON)
	option(TRACY_ON_DEMAND "" ON)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tracy")
endif()

# ---- TINYGLTF ---- #
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tinygltf")

# ---- IMGUI ---- #
# GLFW has no Android backend (see issue #10/#46) - imgui_impl_glfw is excluded there for
# the same reason. imgui_impl_android (issue #51) would be added as its own
# Android-only source here once that lands; until then the Android build gets core imgui
# + the Vulkan backend with no platform backend wired in yet.
if (NOT ANDROID)
	find_package(glfw3 CONFIG REQUIRED)
endif()
find_package(Vulkan REQUIRED)
add_library(imgui STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
)
if (NOT ANDROID)
	target_sources(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp")
endif()
target_include_directories(imgui
	PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan)
if (NOT ANDROID)
	target_link_libraries(imgui PUBLIC glfw)
endif()

# ---- dynalo ---- #
# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dynalo")
