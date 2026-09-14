add_library(engine_filesystem_android STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/AndroidAssetProvider.cpp")
add_library(engine::filesystem::android ALIAS engine_filesystem_android)

target_include_directories(engine_filesystem_android PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(engine_filesystem_android PUBLIC
	android
	engine::filesystem_interface)
