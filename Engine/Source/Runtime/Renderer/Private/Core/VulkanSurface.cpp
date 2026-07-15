#include "Core/VulkanSurface.h"

#include <stdexcept>

#if WE_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace we::runtime::renderer {

std::vector<const char*> GetRequiredVulkanInstanceExtensions() {
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

VkSurfaceKHR CreateVulkanSurface(VkInstance instance, const we::platform::NativeWindowHandle& nativeWindow) {
    if (!IsValid(nativeWindow)) {
        throw std::runtime_error("CreateVulkanSurface: invalid native window handle.");
    }

#if WE_PLATFORM_WINDOWS
    if (nativeWindow.type != we::platform::NativeWindowType::Win32Hwnd) {
        throw std::runtime_error("CreateVulkanSurface: expected Win32 HWND.");
    }
    if (!vkCreateWin32SurfaceKHR) {
        throw std::runtime_error("vkCreateWin32SurfaceKHR is unavailable (volk not loaded?).");
    }

    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = static_cast<HINSTANCE>(nativeWindow.display);
    createInfo.hwnd = static_cast<HWND>(nativeWindow.window);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS || !surface) {
        throw std::runtime_error("vkCreateWin32SurfaceKHR failed.");
    }
    return surface;
#else
    (void)instance;
    throw std::runtime_error("CreateVulkanSurface: platform backend not implemented.");
#endif
}

} // namespace we::runtime::renderer
