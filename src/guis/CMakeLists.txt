add_library(engine_scene_gui STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/SceneGui.cpp")
add_library(engine::scene::gui ALIAS engine_scene_gui)

target_include_directories(engine_scene_gui PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_scene_gui PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_scene_gui PUBLIC engine::logger imgui glm::glm engine::scene)
