add_library(engine_window_android STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/AndroidWindow.cpp")
add_library(engine::window::android ALIAS engine_window_android)

target_include_directories(engine_window_android PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(engine_window_android PUBLIC
	android
	native_app_glue
	engine::window_interface)
