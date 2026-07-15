#include "VulkanBackendShared.h"

namespace we::rhi::vulkan {

VulkanBackendShared& VulkanBackendShared::Get() {
    static VulkanBackendShared instance;
    return instance;
}

} // namespace we::rhi::vulkan
