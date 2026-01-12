add_library(engine_application STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/Application.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ConcurrentQueue.cpp")
add_library(engine::application ALIAS engine_application)

target_include_directories(engine_application PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_application PRIVATE _WIN32_WINNT=0x0A00)
endif()

find_package(asio REQUIRED)
target_link_libraries(engine_application PUBLIC asio::asio engine::logger)
