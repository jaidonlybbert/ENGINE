add_library(engine_window_glfw STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/GlfwWindow.cpp")
add_library(engine::window::glfw ALIAS engine_window_glfw)

target_include_directories(engine_window_glfw PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_window_glfw PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_window_glfw PUBLIC
	glfw
	Vulkan::Vulkan
	engine::window_interface)
