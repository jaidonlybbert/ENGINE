#ifndef ENG_UTILS_DEF
#define ENG_UTILS_DEF
#include <string>
#include <vector>

#include "vulkan/vulkan_core.h"

namespace ENG {
static std::vector<char> readFile(const std::string& filename);
static VkShaderModule createShaderModule(const VkDevice& device, const std::vector<char>& code);
}  // namespace ENG
#endif
