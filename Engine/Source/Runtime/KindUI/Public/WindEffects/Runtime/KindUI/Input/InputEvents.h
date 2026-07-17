#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/Geometry.h"
#include "Platform/InputTypes.h"

namespace WindEffects::Editor::UI {

enum class MouseButton {
    None,
    Left,
    Right,
    Middle
};

enum class MouseEventType {
    MouseDown,
    MouseUp,
    MouseMove,
    MouseWheel
};

struct MouseEvent {
    MouseEventType type = MouseEventType::MouseMove;
    Point position{};
    MouseButton button = MouseButton::None;
    float wheelDeltaX = 0.0f;
    float wheelDeltaY = 0.0f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    bool altDown = false;
    bool shiftDown = false;
    bool ctrlDown = false;
};

enum class KeyEventType {
    KeyDown,
    KeyUp
};

struct KeyEvent {
    KeyEventType type = KeyEventType::KeyDown;
    we::platform::KeyCode key = we::platform::KeyCode::Unknown;
    bool altDown = false;
    bool shiftDown = false;
    bool ctrlDown = false;
};

[[nodiscard]] UI_API char KeyCodeToChar(we::platform::KeyCode key, bool shift);

} // namespace WindEffects::Editor::UI
