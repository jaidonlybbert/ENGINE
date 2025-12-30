# Build config header
set(Engine_INSTALL_DIR ${CMAKE_HOME_DIRECTORY})  # used to resolve paths to assets at runtime
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/EngineConfig.hpp.in EngineConfig.hpp)

add_library(engine_config INTERFACE)
add_library(engine::config ALIAS engine_config)
target_include_directories(engine_config INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
