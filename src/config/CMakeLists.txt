# Build config header
set(Engine_INSTALL_DIR ${CMAKE_HOME_DIRECTORY})  # used to resolve paths to assets at runtime
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/EngineConfig.hpp.in EngineConfig.hpp)

add_library(engine_config STATIC "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeConfig.cpp")
add_library(engine::config ALIAS engine_config)
target_include_directories(engine_config PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	"${PROJECT_SOURCE_DIR}/include")
target_link_libraries(engine_config PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(engine_config PRIVATE engine::logger)
