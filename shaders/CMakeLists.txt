# Compile shaders to SPIR-V with GLSLC
# Find glslc from Vulkan SDK
find_program(GLSLC_EXECUTABLE
    NAMES glslc
    HINTS
        ENV VULKAN_SDK
    PATH_SUFFIXES bin
    DOC "Path to the glslc shader compiler from the Vulkan SDK"
    REQUIRED
)

# Print result
if(GLSLC_EXECUTABLE)
    message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")
else()
    message(FATAL_ERROR "glslc not found. Make sure Vulkan SDK is installed and VULKAN_SDK is set.")
endif()

set(shader_path "${CMAKE_CURRENT_SOURCE_DIR}/")
file(GLOB shaders "${shader_path}*.vert" "${shader_path}*.frag")
foreach(shader ${shaders})
	set(input_glsl "${shader}")
	set(output_spv "${input_glsl}.spv")
	add_custom_command(
		OUTPUT "${output_spv}"
		COMMAND "${GLSLC_EXECUTABLE}" "${input_glsl}" "-o" "${output_spv}"
		DEPENDS "${input_glsl}"
	VERBATIM)
	list(APPEND shader_outputs ${output_spv})
endforeach()
add_custom_target(shader_compile ALL DEPENDS ${shader_outputs} VERBATIM)
target_sources(shader_compile PRIVATE ${shaders})
source_group("Shaders" FILES ${shaders})

cmake_print_variables(shader_path)
cmake_print_variables(shaders)
cmake_print_variables(shader_outputs)
