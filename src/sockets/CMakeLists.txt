add_library(engine_sockets STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/SocketSessionServer.cpp")
add_library(engine::sockets ALIAS engine_sockets)

target_include_directories(engine_sockets PUBLIC "${PROJECT_SOURCE_DIR}/include/sockets")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_sockets PRIVATE _WIN32_WINNT=0x0A00)
endif()

find_package(asio REQUIRED)
target_link_libraries(engine_sockets PUBLIC asio::asio engine::events)
