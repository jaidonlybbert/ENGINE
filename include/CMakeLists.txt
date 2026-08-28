add_library(engine_renderer_interface INTERFACE)

target_sources(engine_renderer_interface INTERFACE
	"${PROJECT_SOURCE_DIR}/include/renderer/RenderAdapterI.hpp"
)

add_library(engine::renderer_interface ALIAS engine_renderer_interface)
