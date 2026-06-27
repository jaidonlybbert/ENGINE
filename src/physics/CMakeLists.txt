add_library(engine_physics STATIC
	"${PROJECT_SOURCE_DIR}/src/physics/physics.cpp"
)
add_library(engine::physics ALIAS engine_physics)

target_include_directories(engine_physics PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_physics PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_physics PUBLIC Jolt::Jolt)
target_link_libraries(engine_physics PUBLIC engine::scene)
target_link_libraries(engine_physics PUBLIC tinygltf)
