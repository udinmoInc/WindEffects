#pragma once

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeToken.h"

#include "Platform/PlatformSDK.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>

namespace we::programs::welauncher {

// Editor-matched chrome metrics (logical px, multiply by LScale at use sites).
// Spacing follows an 8-point grid (4/8/12/16/20/24).
inline constexpr float kLauncherTitleBarH = 36.0f;
inline constexpr float kLauncherLogoDisplaySize = 18.0f;
inline constexpr float kLauncherNavWidth = 212.0f;
inline constexpr float kLauncherNavCollapsedWidth = 60.0f;
inline constexpr float kLauncherNavItemH = 40.0f;
inline constexpr float kLauncherNavPadTop = 16.0f;
inline constexpr float kLauncherNavPadX = 16.0f;
inline constexpr float kLauncherNavItemGap = 8.0f;
inline constexpr float kLauncherNavIconTextGap = 12.0f;
inline constexpr float kLauncherFooterH = 32.0f;
inline constexpr float kLauncherSearchH = 34.0f;
inline constexpr float kLauncherWindowControlW = 40.0f;
inline constexpr float kLauncherHeaderPadL = 16.0f;
inline constexpr float kLauncherContentPadX = 24.0f;
inline constexpr float kLauncherContentPadTop = 24.0f;
inline constexpr float kLauncherContentPadBottom = 24.0f;
inline constexpr float kLauncherTitleToToolbar = 16.0f;
inline constexpr float kLauncherToolbarToDivider = 16.0f;
inline constexpr float kLauncherDividerToTable = 16.0f;
inline constexpr float kLauncherToolbarControlGap = 12.0f;
inline constexpr float kLauncherButtonGap = 8.0f;
inline constexpr float kLauncherSearchW = 320.0f;
inline constexpr float kLauncherRowH = 44.0f;
inline constexpr float kLauncherIconPx = 16.0f;

inline we::runtime::kindui::Color LColor(we::runtime::kindui::ThemeToken token) {
    return we::runtime::kindui::ResolveThemeColor(token);
}

inline float LMetric(we::runtime::kindui::ThemeToken token) {
    return we::runtime::kindui::ResolveThemeMetric(token);
}

inline float LScale() {
    return std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
}

inline void InvalidateUI() {
    we::runtime::kindui::UIRepaintGate::Request();
}

inline std::string FormatRelativeTime(const std::string& isoUtc) {
    if (isoUtc.empty()) {
        return "Never opened";
    }

    // Expect YYYY-MM-DDTHH:MM:SSZ (or without Z)
    std::tm tm{};
    if (isoUtc.size() < 19) {
        return isoUtc;
    }
    try {
        tm.tm_year = std::stoi(isoUtc.substr(0, 4)) - 1900;
        tm.tm_mon = std::stoi(isoUtc.substr(5, 2)) - 1;
        tm.tm_mday = std::stoi(isoUtc.substr(8, 2));
        tm.tm_hour = std::stoi(isoUtc.substr(11, 2));
        tm.tm_min = std::stoi(isoUtc.substr(14, 2));
        tm.tm_sec = std::stoi(isoUtc.substr(17, 2));
    } catch (...) {
        return isoUtc;
    }

#if defined(_WIN32)
    const std::time_t then = _mkgmtime(&tm);
#else
    const std::time_t then = timegm(&tm);
#endif
    if (then < 0) {
        return isoUtc;
    }

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const double seconds = std::difftime(now, then);
    if (seconds < 60.0) {
        return "Just now";
    }
    if (seconds < 3600.0) {
        const int mins = static_cast<int>(seconds / 60.0);
        return std::to_string(mins) + (mins == 1 ? " minute ago" : " minutes ago");
    }
    if (seconds < 86400.0) {
        const int hours = static_cast<int>(seconds / 3600.0);
        return std::to_string(hours) + (hours == 1 ? " hour ago" : " hours ago");
    }
    if (seconds < 86400.0 * 30.0) {
        const int days = static_cast<int>(seconds / 86400.0);
        return std::to_string(days) + (days == 1 ? " day ago" : " days ago");
    }
    return isoUtc.substr(0, 10);
}

inline std::string FormatByteSize(std::uint64_t bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    char buf[48];
    if (unit == 0) {
        std::snprintf(buf, sizeof(buf), "%llu %s", static_cast<unsigned long long>(bytes), units[unit]);
    } else if (value >= 100.0) {
        std::snprintf(buf, sizeof(buf), "%.0f %s", value, units[unit]);
    } else {
        std::snprintf(buf, sizeof(buf), "%.1f %s", value, units[unit]);
    }
    return buf;
}

inline std::string EllipsizePath(const std::string& path, std::size_t maxChars = 48) {
    if (path.size() <= maxChars) {
        return path;
    }
    if (maxChars < 8) {
        return path.substr(0, maxChars);
    }
    const std::size_t head = maxChars / 2 - 2;
    const std::size_t tail = maxChars - head - 3;
    return path.substr(0, head) + "..." + path.substr(path.size() - tail);
}

inline bool RevealInExplorer(const std::string& pathUtf8) {
    if (pathUtf8.empty()) {
        return false;
    }
    auto& platform = we::platform::Platform::Get();
    we::platform::ProcessLaunchDesc desc{};
    desc.executable = "explorer.exe";
    desc.arguments = { "/select," + pathUtf8 };
    desc.detach = true;
    const auto result = platform.LaunchProcess(desc);
    return result && result->launched;
}

inline we::runtime::kindui::Size FixedSize(float w, float h) {
    return we::runtime::kindui::Size{ w, h };
}

} // namespace we::programs::welauncher
