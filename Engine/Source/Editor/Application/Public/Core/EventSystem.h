#pragma once

#include "Application/Export.h"

#include "Core/Geometry.h"
#include <memory>
#include <SDL3/SDL_keycode.h>

namespace we::UI {

class Widget;

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
    MouseEventType type;
    Point position;
    MouseButton button;
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
    KeyEventType type;
    SDL_Keycode keycode;
    bool altDown = false;
    bool shiftDown = false;
    bool ctrlDown = false;
};

class APPLICATION_API EventSystem {
public:
    EventSystem() = default;
    ~EventSystem() = default;

    void SetRootWidget(const std::shared_ptr<Widget>& root) { m_Root = root; }
    std::shared_ptr<Widget> GetRootWidget() const { return m_Root; }

    void ProcessMouseEvent(const MouseEvent& event);
    void ProcessKeyEvent(const KeyEvent& event);

    std::shared_ptr<Widget> GetFocusedWidget() const { return m_FocusedWidget.lock(); }
    std::shared_ptr<Widget> GetHoveredWidget() const { return m_HoveredWidget.lock(); }

    void SetFocusedWidget(const std::shared_ptr<Widget>& widget);
    void SetSuppressSystemCursor(bool suppress) { m_SuppressSystemCursor = suppress; }

    // Hit-testing helper
    static std::shared_ptr<Widget> HitTest(const std::shared_ptr<Widget>& root, const Point& pos);

private:
    void UpdateCursorForWidget(const std::shared_ptr<Widget>& widget, const Point& position);

    std::shared_ptr<Widget> m_Root;
    std::weak_ptr<Widget> m_FocusedWidget;
    std::weak_ptr<Widget> m_HoveredWidget;
    bool m_UsingPointerCursor = false;
    bool m_SuppressSystemCursor = false;
};

} // namespace we::editor::application::UI
