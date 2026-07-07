#pragma once

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL3/SDL.h>
#include "Win32SdlWindow.h"
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

// Apply multi-resolution embedded EXE icon to an SDL top-level window.
// Uses LoadImage at system icon metrics and WM_SETICON for taskbar/title chrome.
inline void ApplyEmbeddedWindowIcon(SDL_Window* window, int resourceId = IDI_ICON1) {
    const HWND hwnd = GetHwndFromSdlWindow(window);
    if (!hwnd) {
        return;
    }

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

inline void ConfigureSdlClassIcons(int resourceId = IDI_ICON1) {
    char idBuffer[16]{};
    SDL_snprintf(idBuffer, sizeof(idBuffer), "%d", resourceId);
    SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON, idBuffer);
    SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON_SMALL, idBuffer);
}

} // namespace we::programs::windows

#endif // _WIN32
