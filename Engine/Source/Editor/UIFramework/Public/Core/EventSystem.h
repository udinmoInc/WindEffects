#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Core/Geometry.h"
#include "Platform/InputTypes.h"
#include <memory>

namespace WindEffects::Editor::UI {

class Widget;
class OverlayHost;

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
    we::platform::KeyCode key = we::platform::KeyCode::Unknown;
    bool altDown = false;
    bool shiftDown = false;
    bool ctrlDown = false;
};

[[nodiscard]] inline char KeyCodeToChar(we::platform::KeyCode key, bool shift) {
    using KC = we::platform::KeyCode;
    if (key >= KC::A && key <= KC::Z) {
        const int i = static_cast<int>(key) - static_cast<int>(KC::A);
        return static_cast<char>((shift ? 'A' : 'a') + i);
    }
    if (key >= KC::Num0 && key <= KC::Num9) {
        return static_cast<char>('0' + (static_cast<int>(key) - static_cast<int>(KC::Num0)));
    }
    switch (key) {
    case KC::Space: return ' ';
    case KC::Minus: return shift ? '_' : '-';
    case KC::Equal: return shift ? '+' : '=';
    case KC::Period: return shift ? '>' : '.';
    case KC::Comma: return shift ? '<' : ',';
    case KC::Slash: return shift ? '?' : '/';
    case KC::Semicolon: return shift ? ':' : ';';
    case KC::Apostrophe: return shift ? '"' : '\'';
    case KC::LeftBracket: return shift ? '{' : '[';
    case KC::RightBracket: return shift ? '}' : ']';
    case KC::Backslash: return shift ? '|' : '\\';
    case KC::Grave: return shift ? '~' : '`';
    default: return '\0';
    }
}

class UIFRAMEWORK_API EventSystem {
public:
    EventSystem() = default;
    ~EventSystem();

    void SetRootWidget(const std::shared_ptr<Widget>& root) { m_Root = root; }
    std::shared_ptr<Widget> GetRootWidget() const { return m_Root; }

    void ProcessMouseEvent(const MouseEvent& event);
    void ProcessKeyEvent(const KeyEvent& event);

    std::shared_ptr<Widget> GetFocusedWidget() const { return m_FocusedWidget.lock(); }
    std::shared_ptr<Widget> GetHoveredWidget() const { return m_HoveredWidget.lock(); }

    void SetFocusedWidget(const std::shared_ptr<Widget>& widget);
    void SetSuppressSystemCursor(bool suppress) { m_SuppressSystemCursor = suppress; }
    
    void SetPopupHost(OverlayHost* popupHost) { m_PopupHost = popupHost; }

    static std::shared_ptr<Widget> HitTest(const std::shared_ptr<Widget>& root, const Point& pos);

private:
    void UpdateCursorForWidget(const std::shared_ptr<Widget>& widget, const Point& position);

#pragma warning(push)
#pragma warning(disable: 4251)
    std::shared_ptr<Widget> m_Root;
    std::weak_ptr<Widget> m_FocusedWidget;
    std::weak_ptr<Widget> m_HoveredWidget;
#pragma warning(pop)
    OverlayHost* m_PopupHost = nullptr;
    bool m_UsingPointerCursor = false;
    bool m_SuppressSystemCursor = false;
};

} // namespace WindEffects::Editor::UI
