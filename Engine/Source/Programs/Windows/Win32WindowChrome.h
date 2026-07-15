#pragma once

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#include "Platform/NativeHandle.h"

#pragma comment(lib, "dwmapi.lib")

namespace we::programs::windows {

inline HWND GetHwnd(const we::platform::NativeWindowHandle& handle) {
    if (handle.type != we::platform::NativeWindowType::Win32Hwnd) {
        return nullptr;
    }
    return static_cast<HWND>(handle.window);
}

// DWM-only chrome: do not use SetWindowRgn — it clips bottom corners and leaves
// a visible gap above the taskbar / system edge.
inline void ConfigureBorderlessWindow(const we::platform::NativeWindowHandle& handle) {
    const HWND hwnd = GetHwnd(handle);
    if (!hwnd) {
        return;
    }

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

    const int cornerPreference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));

    const COLORREF borderColor = RGB(0x30, 0x30, 0x30);
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

inline void UpdateBorderlessWindowShape(const we::platform::NativeWindowHandle& /*handle*/) {
    // Intentionally empty — window region clipping removed to keep the bottom edge flush.
}

} // namespace we::programs::windows

#endif // _WIN32
