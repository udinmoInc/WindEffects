// ==============================================================================
// WindEffects — Platform — WindowsPlatformInternal
// Internal implementation for the Platform module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Windows/WindowsPlatform.h"

namespace we::platform {

inline constexpr const wchar_t* kWindowClassName = L"WindEffects.Platform.Window";

inline void CenterOnMonitor(HWND hwnd, int32_t width, int32_t height, MonitorId /*monitor*/) {
    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi{sizeof(MONITORINFO)};
    if (!GetMonitorInfoW(monitor, &mi)) {
        return;
    }
    const int x = mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - width) / 2;
    const int y = mi.rcWork.top + ((mi.rcWork.bottom - mi.rcWork.top) - height) / 2;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

} // namespace we::platform
