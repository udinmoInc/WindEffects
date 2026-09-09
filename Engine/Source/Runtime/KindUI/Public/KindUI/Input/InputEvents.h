// ==============================================================================
// WindEffects — KindUI — InputEvents
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"
#include "Platform/InputTypes.h"

namespace we::runtime::kindui {

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

[[nodiscard]] KINDUI_API char KeyCodeToChar(we::platform::KeyCode key, bool shift);

} // namespace we::runtime::kindui
