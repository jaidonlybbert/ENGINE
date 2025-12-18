#include<stdio.h>
#include "renderer/vk/Utils.hpp"
#include "vulkan/vulkan_core.h"

namespace ENG {
void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}
}
