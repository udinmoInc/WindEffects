#pragma once

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Platform/NativeHandle.h"
#include "resource.h"

namespace we::programs::windows {

inline HICON LoadSizedIcon(int resourceId, int width, int height) {
    HMODULE module = GetModuleHandleW(nullptr);
    HICON icon = static_cast<HICON>(LoadImageW(
        module,
        MAKEINTRESOURCEW(resourceId),
        IMAGE_ICON,
        width,
        height,
        LR_DEFAULTCOLOR));

    if (!icon) {
        icon = static_cast<HICON>(LoadImageW(
            module,
            MAKEINTRESOURCEW(resourceId),
            IMAGE_ICON,
            0,
            0,
            LR_DEFAULTSIZE));
    }

    return icon;
}

inline void ApplyEmbeddedWindowIcon(const we::platform::NativeWindowHandle& handle, int resourceId = IDI_ICON1) {
    if (handle.type != we::platform::NativeWindowType::Win32Hwnd || !handle.window) {
        return;
    }
    const HWND hwnd = static_cast<HWND>(handle.window);

    const int bigW = GetSystemMetrics(SM_CXICON);
    const int bigH = GetSystemMetrics(SM_CYICON);
    const int smW = GetSystemMetrics(SM_CXSMICON);
    const int smH = GetSystemMetrics(SM_CYSMICON);

    HICON hIconBig = LoadSizedIcon(resourceId, bigW, bigH);
    HICON hIconSm = LoadSizedIcon(resourceId, smW, smH);

    if (!hIconBig) {
        hIconBig = LoadSizedIcon(resourceId, 0, 0);
    }
    if (!hIconSm) {
        hIconSm = LoadSizedIcon(resourceId, 0, 0);
    }

    if (hIconBig) {
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIconBig));
    }
    if (hIconSm) {
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSm));
    }
}

} // namespace we::programs::windows

#endif // _WIN32
