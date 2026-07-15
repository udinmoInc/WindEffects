#pragma once

#include "Platform/Export.h"
#include "Platform/InputTypes.h"
#include "Platform/Types.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace we::platform {

// Strongly typed engine events — OS messages never leave the Platform module.

struct QuitEvent {};

struct WindowCloseEvent {
    WindowId window = WindowId::Invalid;
};

struct WindowFocusEvent {
    WindowId window = WindowId::Invalid;
    bool focused = false;
};

struct WindowMoveEvent {
    WindowId window = WindowId::Invalid;
    Int2 position{};
};

struct WindowResizeEvent {
    WindowId window = WindowId::Invalid;
    Uint2 size{};          // client size (logical points)
    Uint2 pixelSize{};     // client size in pixels (DPI scaled)
};

struct WindowDpiEvent {
    WindowId window = WindowId::Invalid;
    float dpiScale = 1.0f;
    uint32_t dpi = 96;
};

struct WindowMinimizeEvent {
    WindowId window = WindowId::Invalid;
    bool minimized = false;
};

struct WindowMaximizeEvent {
    WindowId window = WindowId::Invalid;
    bool maximized = false;
};

struct KeyEvent {
    WindowId window = WindowId::Invalid;
    KeyCode key = KeyCode::Unknown;
    KeyModifier modifiers = KeyModifier::None;
    bool pressed = false;
    bool repeat = false;
    uint32_t nativeScancode = 0;
};

struct TextInputEvent {
    WindowId window = WindowId::Invalid;
    char32_t codepoint = 0;
};

struct MouseButtonEvent {
    WindowId window = WindowId::Invalid;
    MouseButton button = MouseButton::Left;
    KeyModifier modifiers = KeyModifier::None;
    bool pressed = false;
    Int2 position{};
    uint8_t clickCount = 1;
};

struct MouseMoveEvent {
    WindowId window = WindowId::Invalid;
    Int2 position{};
    Float2 delta{};
    bool relative = false;
};

struct MouseWheelEvent {
    WindowId window = WindowId::Invalid;
    Float2 delta{};
    Int2 position{};
};

struct RawMouseEvent {
    WindowId window = WindowId::Invalid;
    Float2 delta{};
    int32_t buttons = 0;
};

struct CursorEnterEvent {
    WindowId window = WindowId::Invalid;
    bool entered = false;
};

struct DropFilesEvent {
    WindowId window = WindowId::Invalid;
    Int2 position{};
    std::vector<std::string> paths{};
};

struct GamepadConnectionEvent {
    uint32_t gamepadIndex = 0;
    bool connected = false;
    std::string name;
};

struct GamepadButtonEvent {
    uint32_t gamepadIndex = 0;
    GamepadButton button = GamepadButton::A;
    bool pressed = false;
};

struct GamepadAxisEvent {
    uint32_t gamepadIndex = 0;
    GamepadAxis axis = GamepadAxis::LeftX;
    float value = 0.0f;
};

struct TouchEvent {
    WindowId window = WindowId::Invalid;
    uint64_t fingerId = 0;
    Float2 position{};
    Float2 delta{};
    float pressure = 1.0f;
    bool pressed = false;
    bool moved = false;
    bool released = false;
};

struct DirectoryChangeEvent {
    DirectoryWatcherId watcher = DirectoryWatcherId::Invalid;
    DirectoryWatchAction action = DirectoryWatchAction::Modified;
    std::string path;
    std::string oldPath;
};

struct TimerEvent {
    TimerId timer = TimerId::Invalid;
};

using PlatformEvent = std::variant<
    QuitEvent,
    WindowCloseEvent,
    WindowFocusEvent,
    WindowMoveEvent,
    WindowResizeEvent,
    WindowDpiEvent,
    WindowMinimizeEvent,
    WindowMaximizeEvent,
    KeyEvent,
    TextInputEvent,
    MouseButtonEvent,
    MouseMoveEvent,
    MouseWheelEvent,
    RawMouseEvent,
    CursorEnterEvent,
    DropFilesEvent,
    GamepadConnectionEvent,
    GamepadButtonEvent,
    GamepadAxisEvent,
    TouchEvent,
    DirectoryChangeEvent,
    TimerEvent
>;

} // namespace we::platform
