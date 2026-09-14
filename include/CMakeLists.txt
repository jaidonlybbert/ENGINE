add_library(engine_renderer_interface INTERFACE)

target_sources(engine_renderer_interface INTERFACE
	"${PROJECT_SOURCE_DIR}/include/renderer/RenderAdapterI.hpp"
	"${PROJECT_SOURCE_DIR}/include/renderer/vk/pipelines/PipelineFactoryI.hpp"
)

add_library(engine::renderer_interface ALIAS engine_renderer_interface)

# Abstract windowing/input/filesystem interfaces (see issue #42) - each has exactly one
# implementation today (GLFW for window/hid, the local filesystem for assets), registered
# as its own header-only target the same way engine_renderer_interface is, so a future
# platform-specific implementation can depend on just the interface without pulling in
# GLFW/the desktop filesystem layout.
add_library(engine_window_interface INTERFACE)
target_sources(engine_window_interface INTERFACE
	"${PROJECT_SOURCE_DIR}/include/window/WindowI.hpp"
)
add_library(engine::window_interface ALIAS engine_window_interface)

add_library(engine_hid_interface INTERFACE)
target_sources(engine_hid_interface INTERFACE
	"${PROJECT_SOURCE_DIR}/include/hid/InputI.hpp"
)
add_library(engine::hid_interface ALIAS engine_hid_interface)

add_library(engine_filesystem_interface INTERFACE)
target_sources(engine_filesystem_interface INTERFACE
	"${PROJECT_SOURCE_DIR}/include/filesystem/AssetProviderI.hpp"
)
add_library(engine::filesystem_interface ALIAS engine_filesystem_interface)
