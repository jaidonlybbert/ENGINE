add_library(engine_hid_glfw STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/GlfwInput.cpp")
add_library(engine::hid::glfw ALIAS engine_hid_glfw)

target_include_directories(engine_hid_glfw PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_hid_glfw PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_hid_glfw PUBLIC
	glfw
	engine::hid_interface
	engine::window::glfw
	engine::events
	engine::logger)
