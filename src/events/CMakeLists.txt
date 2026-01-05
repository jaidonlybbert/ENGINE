add_library(engine_events STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/Event.cpp")
add_library(engine::events ALIAS engine_events)

target_include_directories(engine_events PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_events PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_events PUBLIC nlohmann_json::nlohmann_json)
