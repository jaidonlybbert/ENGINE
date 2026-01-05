add_library(engine_vk_renderer STATIC
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Buffer.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Command.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Device.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Image.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Instance.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/PhysicalDevice.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Renderer.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Swapchain.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/Utils.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/pipelines/Pipeline.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/pipelines/PipelineFactory.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/pipelines/PipelinePosColTex.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/pipelines/PipelineUtils.cpp"
	"${PROJECT_SOURCE_DIR}/src/renderer/vk/pipelines/ShaderFactory.cpp"
)
add_library(engine::vk::renderer ALIAS engine_vk_renderer)

target_include_directories(engine_vk_renderer PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	target_link_libraries(engine_vk_renderer PUBLIC Tracy::TracyClient)
	# Target Windows 10 or later
	target_compile_definitions(engine_vk_renderer PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_vk_renderer PUBLIC Vulkan::Vulkan)
target_link_libraries(engine_vk_renderer PUBLIC engine::filesystem)
target_link_libraries(engine_vk_renderer PUBLIC tinygltf)
target_link_libraries(engine_vk_renderer PUBLIC engine::logger)
target_link_libraries(engine_vk_renderer PUBLIC tinyobjloader::tinyobjloader)
target_link_libraries(engine_vk_renderer PUBLIC engine::scene)
target_link_libraries(engine_vk_renderer PUBLIC glfw)
target_link_libraries(engine_vk_renderer PUBLIC imgui)
target_link_libraries(engine_vk_renderer PUBLIC GPUOpen::VulkanMemoryAllocator)
