  add_library(engine_logger INTERFACE)
  add_library(engine::logger ALIAS engine_logger)
  target_include_directories(engine_logger INTERFACE "${PROJECT_SOURCE_DIR}/include/logger")
  target_link_libraries(engine_logger INTERFACE spdlog::spdlog)
