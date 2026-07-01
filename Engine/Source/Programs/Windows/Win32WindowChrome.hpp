#pragma once

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#include <SDL3/SDL.h>
#include "Win32SdlWindow.hpp"

#pragma comment(lib, "dwmapi.lib")

namespace we::programs::windows {

// DWM-only chrome: do not use SetWindowRgn — it clips bottom corners and leaves
// a visible gap above the taskbar / system edge.
inline void ConfigureBorderlessWindow(SDL_Window* window) {
    const HWND hwnd = GetHwndFromSdlWindow(window);
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

inline void UpdateBorderlessWindowShape(SDL_Window* /*window*/) {
    // Intentionally empty — window region clipping removed to keep the bottom edge flush.
}

} // namespace we::programs::windows

#endif // _WIN32
