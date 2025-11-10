#include<stdexcept>
#include "vulkan/vulkan_core.h"

namespace ENG {
template<typename Tfrom, typename Tto>
Tto* checked_cast(Tfrom* base) {
    Tto* ptr = dynamic_cast<Tto*>(base);
    if (!ptr) throw std::runtime_error("Invalid cast");
    return ptr;
}

void check_vk_result(VkResult err);
}
