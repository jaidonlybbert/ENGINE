// VulkanMemoryAllocator (VMA) is header-only, so exactly one translation unit across the
// whole link must define VMA_IMPLEMENTATION. The desktop Engine target gets this from
// src/HeaderLibs.cpp, but the separate Android Engine target never compiles that file (see
// src/scene/GltfImpl.cpp for the same situation with tinygltf). This went unnoticed until
// now because nothing on the Android call graph called into VMA-using code (VkAdapter,
// Buffer.cpp) before issue #59 wired up a real, persistent renderer. Guarded to __ANDROID__
// so this compiles to an empty TU elsewhere and never collides with HeaderLibs.cpp's own
// VMA_IMPLEMENTATION on desktop.
#ifdef __ANDROID__

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#endif  // __ANDROID__
