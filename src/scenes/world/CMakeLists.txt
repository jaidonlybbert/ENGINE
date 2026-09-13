add_library(scene_world STATIC
	"${PROJECT_SOURCE_DIR}/src/scenes/world/SceneWorld.cpp"
)
add_library(engine::scenes::world ALIAS scene_world)

target_include_directories(scene_world PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(scene_world PRIVATE _WIN32_WINNT=0x0A00)
endif()

# Self-contained: only depends on engine libraries and the scenes/common helpers. Does not
# depend on any other individual scene, so it can be linked without pulling one in.
target_link_libraries(scene_world PUBLIC engine::renderer_interface)
target_link_libraries(scene_world PUBLIC engine::scene)
target_link_libraries(scene_world PUBLIC engine::filesystem)
target_link_libraries(scene_world PUBLIC engine::logger)
target_link_libraries(scene_world PUBLIC pmp::pmp)
target_link_libraries(scene_world PUBLIC engine::scenes::common)
