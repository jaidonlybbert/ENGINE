add_library(engine_gui STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/Gui.cpp")
add_library(engine::gui ALIAS engine_gui)

target_include_directories(engine_gui PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_gui PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_gui PUBLIC engine::logger imgui glm::glm)
