add_library(scenes_common STATIC
	"${PROJECT_SOURCE_DIR}/src/scenes/common/CameraControls.cpp"
	"${PROJECT_SOURCE_DIR}/src/scenes/common/ProceduralGeometry.cpp"
)
add_library(engine::scenes::common ALIAS scenes_common)

target_include_directories(scenes_common PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(scenes_common PRIVATE _WIN32_WINNT=0x0A00)
endif()

# Shared by any scene - camera controls and procedural mesh helpers are generic, not tied
# to a particular scene's contents. No scene library depends on another scene library;
# everything they share in common lives here instead.
target_link_libraries(scenes_common PUBLIC engine::scene)
target_link_libraries(scenes_common PUBLIC engine::hid)
target_link_libraries(scenes_common PUBLIC engine::logger)
target_link_libraries(scenes_common PUBLIC pmp::pmp)
