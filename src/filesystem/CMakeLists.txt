add_library(engine_filesystem STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/FilesystemInterface.cpp")
add_library(engine::filesystem ALIAS engine_filesystem)

target_include_directories(engine_filesystem PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_filesystem PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_filesystem PUBLIC engine::config)
