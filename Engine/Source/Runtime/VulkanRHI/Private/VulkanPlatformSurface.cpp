#include "VulkanPlatformSurface.h"

#include "Platform/PlatformConfig.h"

#include <stdexcept>
#include <string>

#if WE_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "Platform/UndefWin32Macros.h"
#endif

namespace we::rhi::vulkan {

std::vector<const char*> GetRequiredInstanceExtensions() {
    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if WE_PLATFORM_WINDOWS
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif WE_PLATFORM_LINUX
    extensions.push_back("VK_KHR_xcb_surface");
#elif WE_PLATFORM_MAC || WE_PLATFORM_IOS
    extensions.push_back("VK_EXT_metal_surface");
#elif WE_PLATFORM_ANDROID
    extensions.push_back("VK_KHR_android_surface");
#endif
    return extensions;
}

VkSurfaceKHR CreateSurface(VkInstance instance, const we::platform::NativeWindowHandle& nativeWindow) {
    if (!we::platform::IsValid(nativeWindow)) {
        throw std::runtime_error("VulkanRHI CreateSurface: invalid native window handle.");
    }

#if WE_PLATFORM_WINDOWS
    if (nativeWindow.type != we::platform::NativeWindowType::Win32Hwnd) {
        throw std::runtime_error(
            "VulkanRHI CreateSurface: expected Win32 HWND (type="
            + std::to_string(static_cast<uint32_t>(nativeWindow.type)) + ").");
    }
    if (!vkCreateWin32SurfaceKHR) {
        throw std::runtime_error("VulkanRHI: vkCreateWin32SurfaceKHR unavailable (volk not loaded?).");
    }

    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = static_cast<HINSTANCE>(nativeWindow.display);
    createInfo.hwnd = static_cast<HWND>(nativeWindow.window);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS || !surface) {
        throw std::runtime_error("VulkanRHI: vkCreateWin32SurfaceKHR failed.");
    }
    return surface;
#else
    (void)instance;
    throw std::runtime_error("VulkanRHI CreateSurface: platform not implemented.");
#endif
}

} // namespace we::rhi::vulkan
