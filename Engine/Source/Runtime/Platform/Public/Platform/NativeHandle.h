#pragma once

#include "Platform/Export.h"
#include "Platform/Types.h"

#include <cstdint>

namespace we::platform {

// Minimal native objects for RHI surface creation.
// Contains NO Vulkan / DirectX / Metal / OpenGL types or logic.

enum class NativeWindowType : uint8_t {
    None = 0,
    Win32Hwnd,
    X11Window,
    WaylandSurface,
    CocoaNSWindow,
    AndroidNativeWindow,
    UIKitUIView
};

struct NativeWindowHandle {
    NativeWindowType type = NativeWindowType::None;

    // Primary window surface (HWND, NSWindow*, wl_surface*, ANativeWindow*, UIView*, X11 Window as ptr).
    void* window = nullptr;

    // Display / connection (HINSTANCE, Display*, wl_display*, NSApplication*, etc.). Optional.
    void* display = nullptr;

    // Extra (X11 visual/root, Wayland compositor, etc.). Optional.
    void* extra = nullptr;

    uint64_t windowId = 0; // X11 Window numeric id when applicable
};

[[nodiscard]] inline bool IsValid(const NativeWindowHandle& h) noexcept {
    return h.type != NativeWindowType::None && h.window != nullptr;
}

} // namespace we::platform
