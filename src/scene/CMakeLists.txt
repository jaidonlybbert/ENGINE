add_library(engine_scene STATIC
	"${PROJECT_SOURCE_DIR}/src/scene/DFT.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/DrawData.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/Gltf.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/Mesh.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/Mesh2.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/Obj.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/Scene.cpp"
	"${PROJECT_SOURCE_DIR}/src/scene/VulkanAdapter.cpp"
)
add_library(engine::scene ALIAS engine_scene)

target_include_directories(engine_scene PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_scene PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_scene PUBLIC Vulkan::Vulkan)
target_link_libraries(engine_scene PUBLIC engine::filesystem)
target_link_libraries(engine_scene PUBLIC tinygltf)
target_link_libraries(engine_scene PUBLIC engine::logger)
target_link_libraries(engine_scene PUBLIC tinyobjloader::tinyobjloader)
target_link_libraries(engine_scene PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(engine_scene PUBLIC glm::glm)

