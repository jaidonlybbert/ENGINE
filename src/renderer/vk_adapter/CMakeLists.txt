add_library(engine_vk_adapter STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/VkAdapter.cpp")
add_library(engine::vk::adapter ALIAS engine_vk_adapter)

target_include_directories(engine_vk_adapter PUBLIC "${PROJECT_SOURCE_DIR}/include/")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_vk_adapter PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(engine_vk_adapter PUBLIC Vulkan::Vulkan engine::scene engine::vk::renderer)
