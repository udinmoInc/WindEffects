#pragma once

#include "Platform/Export.h"

#include <cstdint>

namespace we::platform {

// Feature bits reported by the host backend. Query these instead of
// platform #ifdefs in Engine / Editor / Tools.
struct PLATFORM_API PlatformCapabilities {
    bool multipleWindows = false;
    bool highDpi = false;
    bool perMonitorDpi = false;
    bool borderlessWindow = false;
    bool fullscreen = false;
    bool rawMouse = false;
    bool relativeMouse = false;
    bool highResolutionMouse = false;
    bool gamepads = false;
    bool touch = false;
    bool clipboard = false;
    bool fileDialogs = false;
    bool messageBoxes = false;
    bool dragAndDrop = false;
    bool directoryWatching = false;
    bool darkMode = false;
    bool systemTray = false;
    bool multipleMonitors = false;
    bool threading = true;
    bool dynamicLibraries = true;
    bool highResolutionTimer = true;
    bool console = false;
    bool crashDumps = false;

    [[nodiscard]] bool SupportsRawMouse() const noexcept { return rawMouse; }
    [[nodiscard]] bool SupportsMultipleWindows() const noexcept { return multipleWindows; }
    [[nodiscard]] bool SupportsHighDPI() const noexcept { return highDpi; }
    [[nodiscard]] bool SupportsPerMonitorDPI() const noexcept { return perMonitorDpi; }
    [[nodiscard]] bool SupportsBorderlessWindow() const noexcept { return borderlessWindow; }
    [[nodiscard]] bool SupportsGamepads() const noexcept { return gamepads; }
    [[nodiscard]] bool SupportsTouch() const noexcept { return touch; }
    [[nodiscard]] bool SupportsDirectoryWatching() const noexcept { return directoryWatching; }
    [[nodiscard]] bool SupportsDarkMode() const noexcept { return darkMode; }
    [[nodiscard]] bool SupportsSystemTray() const noexcept { return systemTray; }
    [[nodiscard]] bool SupportsClipboard() const noexcept { return clipboard; }
    [[nodiscard]] bool SupportsFileDialogs() const noexcept { return fileDialogs; }
    [[nodiscard]] bool SupportsMultipleMonitors() const noexcept { return multipleMonitors; }
};

// Independent service groups so tools / headless apps can enable only what they need.
enum class PlatformService : uint32_t {
    None            = 0,
    Windowing       = 1u << 0,
    Input           = 1u << 1,
    Dialogs         = 1u << 2,
    FileSystem      = 1u << 3,
    Threading       = 1u << 4,
    Timers          = 1u << 5,
    DirectoryWatch  = 1u << 6,
    Diagnostics     = 1u << 7,
    Crash           = 1u << 8,
    All             = 0xFFFFFFFFu
};

[[nodiscard]] constexpr PlatformService operator|(PlatformService a, PlatformService b) noexcept {
    return static_cast<PlatformService>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

[[nodiscard]] constexpr PlatformService operator&(PlatformService a, PlatformService b) noexcept {
    return static_cast<PlatformService>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

[[nodiscard]] constexpr bool HasService(PlatformService mask, PlatformService bit) noexcept {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(bit)) != 0;
}

} // namespace we::platform
