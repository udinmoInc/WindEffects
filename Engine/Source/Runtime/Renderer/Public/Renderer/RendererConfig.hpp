#pragma once

#if WE_HAS_VULKAN
#include <volk.h>
#endif

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

// Offscreen viewport color target: HDR linear during scene rendering, tonemapped before UI sampling.
inline constexpr VkFormat kOffscreenColorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
inline constexpr VkFormat kOffscreenDepthFormat = VK_FORMAT_D32_SFLOAT;

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
