#pragma once

#include "Platform/NativeHandle.h"
#include "Platform/PlatformConfig.h"

#include <vector>

#if WE_PLATFORM_WINDOWS
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#endif

#include <volk.h>

namespace we::rhi::vulkan {

[[nodiscard]] std::vector<const char*> GetRequiredInstanceExtensions();
[[nodiscard]] VkSurfaceKHR CreateSurface(VkInstance instance, const we::platform::NativeWindowHandle& nativeWindow);

} // namespace we::rhi::vulkan
