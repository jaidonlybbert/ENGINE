add_library(engine_renderer_interface INTERFACE)

target_sources(engine_renderer_interface INTERFACE
	"${PROJECT_SOURCE_DIR}/include/renderer/RenderAdapterI.hpp"
	"${PROJECT_SOURCE_DIR}/include/renderer/vk/pipelines/PipelineFactoryI.hpp"
)

add_library(engine::renderer_interface ALIAS engine_renderer_interface)
