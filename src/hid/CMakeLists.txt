add_library(engine_hid STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/Input.cpp")
add_library(engine::hid ALIAS engine_hid)

target_include_directories(engine_hid PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_hid PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_hid PUBLIC glfw engine::scene engine::logger)
