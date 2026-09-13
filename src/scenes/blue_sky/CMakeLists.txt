add_library(scene_blue_sky STATIC
	"${PROJECT_SOURCE_DIR}/src/scenes/blue_sky/SceneBlueSky.cpp"
)
add_library(engine::scenes::blue_sky ALIAS scene_blue_sky)

target_include_directories(scene_blue_sky PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(scene_blue_sky PRIVATE _WIN32_WINNT=0x0A00)
endif()

# Self-contained: only depends on engine libraries and the scenes/common helpers. Does not
# depend on any other individual scene, so it can be linked without pulling one in.
target_link_libraries(scene_blue_sky PUBLIC engine::renderer_interface)
target_link_libraries(scene_blue_sky PUBLIC engine::scene)
target_link_libraries(scene_blue_sky PUBLIC engine::logger)
target_link_libraries(scene_blue_sky PUBLIC engine::scenes::common)
