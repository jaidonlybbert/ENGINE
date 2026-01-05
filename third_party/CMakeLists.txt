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
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
add_library(imgui STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
)
target_include_directories(imgui
	PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan)
target_link_libraries(imgui PUBLIC glfw)

# ---- dynalo ---- #
# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dynalo")
