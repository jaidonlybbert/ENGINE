if (WIN32)
	# Tracy profiler doesn't build on ARM64 MacOS, the client does build however,
	# so maybe another machine could be used for the profiler over the network
	option(TRACY_ENABLE "" ON)
	option(TRACY_ON_DEMAND "" ON)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tracy")
endif()

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tinygltf")
