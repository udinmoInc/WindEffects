#pragma once

#include "Platform/Export.h"

#include <cstdint>
#include <string_view>

namespace we::platform {

enum class KeyCode : uint16_t {
    Unknown = 0,

    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,

    Escape, Tab, CapsLock, Space, Enter, Backspace, Insert, Delete,
    Home, End, PageUp, PageDown,
    Left, Right, Up, Down,

    LeftShift, RightShift, LeftControl, RightControl,
    LeftAlt, RightAlt, LeftSuper, RightSuper, Menu,

    PrintScreen, ScrollLock, Pause, NumLock,

    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadDecimal, NumpadDivide, NumpadMultiply, NumpadSubtract, NumpadAdd, NumpadEnter,

    Apostrophe, Comma, Minus, Period, Slash, Semicolon, Equal,
    LeftBracket, Backslash, RightBracket, Grave,

    Count
};

enum class MouseButton : uint8_t {
    Left = 0,
    Right,
    Middle,
    X1,
    X2,
    Count
};

enum class KeyModifier : uint8_t {
    None         = 0,
    Shift        = 1 << 0,
    Control      = 1 << 1,
    Alt          = 1 << 2,
    Super        = 1 << 3,
    CapsLock     = 1 << 4,
    NumLock      = 1 << 5
};

[[nodiscard]] constexpr KeyModifier operator|(KeyModifier a, KeyModifier b) noexcept {
    return static_cast<KeyModifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

[[nodiscard]] constexpr KeyModifier operator&(KeyModifier a, KeyModifier b) noexcept {
    return static_cast<KeyModifier>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

[[nodiscard]] constexpr bool HasFlag(KeyModifier mask, KeyModifier flag) noexcept {
    return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(flag)) != 0;
}

enum class GamepadButton : uint8_t {
    A, B, X, Y,
    Back, Guide, Start,
    LeftStick, RightStick,
    LeftShoulder, RightShoulder,
    DPadUp, DPadDown, DPadLeft, DPadRight,
    Count
};

enum class GamepadAxis : uint8_t {
    LeftX, LeftY, RightX, RightY,
    LeftTrigger, RightTrigger,
    Count
};

[[nodiscard]] PLATFORM_API const char* ToString(KeyCode key) noexcept;
[[nodiscard]] PLATFORM_API KeyCode KeyCodeFromName(std::string_view name) noexcept;

} // namespace we::platform
