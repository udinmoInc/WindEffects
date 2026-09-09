// ==============================================================================
// WindEffects — VulkanRHI — VulkanPlatformSurface
// Internal implementation for the VulkanRHI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
