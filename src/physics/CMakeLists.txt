add_library(engine_physics STATIC
	"${PROJECT_SOURCE_DIR}/src/physics/physics.cpp"
)
add_library(engine::physics ALIAS engine_physics)

target_include_directories(engine_physics PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
	# Target Windows 10 or later
	target_compile_definitions(engine_physics PRIVATE _WIN32_WINNT=0x0A00)
endif()

# The Jolt package from Conan is built without C++ RTTI (CPP_RTTI_ENABLED=OFF -> -fno-rtti / /GR-).
# physics.cpp derives from Jolt's abstract interface classes, so this TU must be compiled with the
# same setting. Otherwise Clang emits references to Jolt's base-class type_info that libJolt never
# defines, and the link fails with "undefined reference to `typeinfo for JPH::...`". (GCC still
# emits the base type_info under -fno-rtti, which is why only the Clang build breaks.)
# Jolt has its own lightweight RTTI system and does not rely on compiler RTTI.
if (MSVC)
	target_compile_options(engine_physics PRIVATE /GR-)
else()
	target_compile_options(engine_physics PRIVATE -fno-rtti)
endif()

target_link_libraries(engine_physics PUBLIC Jolt::Jolt)
target_link_libraries(engine_physics PUBLIC engine::scene)
target_link_libraries(engine_physics PUBLIC tinygltf)
