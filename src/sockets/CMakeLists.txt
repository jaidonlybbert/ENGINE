add_library(sockets STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/SocketSessionServer.cpp")

target_include_directories(sockets PUBLIC "${PROJECT_SOURCE_DIR}/include/sockets")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(sockets PRIVATE _WIN32_WINNT=0x0A00)
endif()

find_package(asio REQUIRED)
target_link_libraries(sockets PUBLIC asio::asio)
