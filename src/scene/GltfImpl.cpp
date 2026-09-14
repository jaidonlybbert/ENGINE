// tinygltf is header-only (TINYGLTF_HEADER_ONLY=ON, third_party/tinygltf/CMakeLists.txt),
// so exactly one translation unit across the whole link must define TINYGLTF_IMPLEMENTATION.
// The desktop Engine target gets this from src/HeaderLibs.cpp, but the separate Android
// Engine target (see root CMakeLists.txt's `if (ANDROID)` branch) never compiles that file
// - see issue #50, where Gltf.cpp's symbols first became actually referenced from Android
// code (main_android.cpp's verifyAssetLoading()), surfacing this as a link failure. Guarded
// to __ANDROID__ so this compiles to an empty TU elsewhere and never collides with
// HeaderLibs.cpp's own TINYGLTF_IMPLEMENTATION on desktop.
#ifdef __ANDROID__

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#endif  // __ANDROID__
