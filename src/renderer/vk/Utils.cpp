#include "renderer/vk/Utils.hpp"

#include <iostream>

#include "logger/Logging.hpp"
#include "vulkan/vulkan_core.h"

namespace ENG {
void check_vk_result(VkResult err) {
    if (err == VK_SUCCESS) return;
    ENG_LOG_ERROR("[vulkan] Error: VkResult = " << err);
    if (err < 0) abort();
}
}  // namespace ENG
