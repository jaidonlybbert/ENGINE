add_library(scenes STATIC
	"${PROJECT_SOURCE_DIR}/src/scenes/ProceduralGeometry.cpp"
	"${PROJECT_SOURCE_DIR}/src/scenes/SceneWorld.cpp"
)

target_include_directories(scenes PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(scenes PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(scenes PUBLIC engine::scene)
target_link_libraries(scenes PUBLIC engine::vk::renderer)
target_link_libraries(scenes PUBLIC engine::filesystem)
target_link_libraries(scenes PUBLIC engine::logger)
target_link_libraries(scenes PUBLIC pmp::pmp)
