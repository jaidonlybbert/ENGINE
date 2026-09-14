add_library(engine_hid_android STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/AndroidInput.cpp")
add_library(engine::hid::android ALIAS engine_hid_android)

target_include_directories(engine_hid_android PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(engine_hid_android PUBLIC
	android
	engine::hid_interface
	engine::events)
