#pragma once

#include "Platform/NativeHandle.h"
#include "Platform/PlatformConfig.h"

#if WE_PLATFORM_WINDOWS
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#endif

#include <volk.h>
#include <vector>

namespace we::runtime::renderer {

[[nodiscard]] std::vector<const char*> GetRequiredVulkanInstanceExtensions();
[[nodiscard]] VkSurfaceKHR CreateVulkanSurface(VkInstance instance, const we::platform::NativeWindowHandle& nativeWindow);

} // namespace we::runtime::renderer
