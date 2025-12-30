add_library(application STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/Application.cpp")

target_include_directories(application PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(application PRIVATE _WIN32_WINNT=0x0A00)
endif()

find_package(asio REQUIRED)
target_link_libraries(application PUBLIC asio::asio logger)
